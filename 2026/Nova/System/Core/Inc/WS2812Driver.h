/*
 * WS2812Driver.h
 *
 *  Created on: Feb 28, 2026
 *      Author: Mohamed Gdoura
 *
 *  Bit-banged-over-PWM/DMA driver for WS2812B addressable LEDs.
 *  (Renamed from Adafruit_NeoPixel_STM; statically allocated: no heap.)
 *
 * ===========================================================================
 * HOW IT WORKS, and why it costs the CPU nothing
 * ===========================================================================
 *
 * A WS2812B has no clock line. Every bit is one fixed 1.25 us slot, and what
 * says 0 from 1 is HOW LONG THE LINE STAYS HIGH inside that slot:
 *
 *     '0'  high ~350 ns, then low     -> CCR_B0
 *     '1'  high ~700 ns, then low     -> CCR_B1
 *
 * 24 bits per LED (GRB), sent back to back. A gap longer than ~50 us anywhere
 * in the frame and the strip latches early, so the whole thing is one
 * uninterruptible burst: 72 LEDs = 1728 bits = 2.16 ms of hard real time.
 *
 * Doing that in software means hitting a deadline every 1.25 us for two
 * milliseconds - impossible under FreeRTOS, and the reason naive drivers
 * disable interrupts for the whole frame and wreck everything else's timing.
 *
 * So the hardware does it instead:
 *
 *   1. The timer runs at exactly 800 kHz (ARR_PERIOD), PWM mode. Its CCR sets
 *      the high time of the current slot - which IS the bit being sent.
 *   2. _pBuff[] holds one CCR value per bit: CCR_B0 or CCR_B1, filled in by
 *      setPixelColor().
 *   3. A DMA channel is tied to this channel's capture/compare request. Every
 *      slot the counter wraps, DMA copies the next _pBuff entry into CCR. No
 *      CPU, no interrupt, no jitter - the transfer is memory-to-peripheral.
 *
 * show() therefore starts the transfer and RETURNS - a few microseconds of
 * CPU. The 2.2 ms of streaming happens in hardware while every thread keeps
 * running. When the DMA count reaches zero the transfer-complete interrupt
 * fires, reaches onFrameCompleteISR() through Bridge, and gives _frameDone.
 *
 * _frameDone is what makes that safe: binary, given = idle. show() takes it
 * without blocking, so a call landing mid-frame SKIPS rather than waiting or
 * corrupting the buffer being streamed.
 *
 *     show()      [take sem] -> [start DMA] -> return          (~us of CPU)
 *                                   |
 *          hardware streams 1778 slots, 2.2 ms, CPU free
 *                                   |
 *     DMA TC IRQ -> Bridge -> onFrameCompleteISR -> [give sem]
 *
 * The last RESET_PULSE slots carry CCR = 0, holding the line low past the
 * 50 us latch window - and leaving CCR at 0, so the pad idles low afterwards.
 */

#pragma once

#include "stm32g4xx_hal.h"
#include <math.h>
#include <cstdint>
#include <array>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "semphr.h"

// ---------------------------------------------------------------------------
// Bit timing, derived at compile time from the timer input clock.
//
// ASSUMPTIONS - checked by static_assert where the compiler can see them,
// and at begin() (clockOk) where it can't:
//   1. The timer's input clock is WS2812_TIM_CLK_HZ. Here: SYSCLK 144 MHz
//      (48 MHz HSE /4 *24 /2) with AHB and APB2 prescalers at /1, so the
//      TIM15 kernel clock equals SYSCLK. Verified at runtime in begin().
//   2. Timer prescaler is 0 — ENFORCED by begin(), which writes PSC and ARR
//      and latches them with an update event. Necessary because PWMDriver
//      (servo configs) shares this timer handle on other board roles and
//      reprograms PSC to a 1 MHz base at construction.
//   3. WS2812B datasheet windows: T0H 220-380 ns, T1H 580-1000 ns,
//      bit period 1.25 us, latch/reset low held > 50 us.
// begin() forces ARR to ARR_PERIOD so the CubeMX TIM15 Period does not matter.
// ---------------------------------------------------------------------------
inline constexpr uint32_t WS2812_TIM_CLK_HZ = 144000000u;
inline constexpr uint32_t WS2812_BIT_HZ     = 800000u;    // 1.25 us bit period

/** Nanoseconds -> timer ticks at WS2812_TIM_CLK_HZ, rounded to nearest. */
inline constexpr uint32_t ws2812NsToTicks(uint32_t ns) {
	return (uint32_t)(((uint64_t)WS2812_TIM_CLK_HZ * ns + 500000000ULL) / 1000000000ULL);
}
/** Timer ticks -> nanoseconds; how the spec-window asserts below read a CCR. */
inline constexpr uint32_t ws2812TicksToNs(uint32_t ticks) {
	return (uint32_t)(((uint64_t)ticks * 1000000000ULL) / WS2812_TIM_CLK_HZ);
}


inline constexpr uint32_t ARR_PERIOD = WS2812_TIM_CLK_HZ / WS2812_BIT_HZ - 1; // 179 @ 144 MHz
inline constexpr uint32_t CCR_B0 = ws2812NsToTicks(350); // 0-bit high time (50 @ 144 MHz)
inline constexpr uint32_t CCR_B1 = ws2812NsToTicks(700); // 1-bit high time (101 @ 144 MHz)


inline constexpr uint16_t BITS_PER_LED = 24;
inline constexpr uint16_t RESET_PULSE  = 50; // zero-duty tail slots appended to each frame


// Strip length: sizes the static buffers (no heap) and is what the code drives.
inline constexpr uint16_t WS2812_MAX_LEDS   = 72;
inline constexpr uint16_t WS2812_MAX_BUFFER = WS2812_MAX_LEDS * BITS_PER_LED + RESET_PULSE;


/**
 * Supply ceiling, in the units a WS2812 channel is driven in.
 *
 * Was a cap on a global brightness scale, back when there was one: 255 meant a
 * full-white strip, which the supply cannot carry, so the scale was clamped here.
 * Colours are now written through unscaled, so the same limit is expressed where
 * it actually bites - as a per-pixel budget, since what the supply sees is the
 * sum of the three channels, not any one of them. A pixel may therefore be
 * (0,255,0) at 255 units while full white at 200 each is 600. DO NOT EXCEED.
 */
inline constexpr uint16_t BRIGHTNESS_SAFETY_THRESH = 200;
inline constexpr uint16_t PIXEL_BUDGET = 3 * BRIGHTNESS_SAFETY_THRESH;

// Array containing RGB values. 0 = R | 1 = G | 2 = B.
struct Color {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
};

/** Current a pixel asks of the supply, as channel units summed. */
/** Current a pixel asks of the supply: the three channels summed, since that is
 *  what the rail sees. Compare against PIXEL_BUDGET. */
inline constexpr uint16_t pixelLoad(const Color& color) { return color.r + color.g + color.b; }

/** Pack a Color into the 24 bits the strip expects, GRB order, G first. */
uint32_t colorCode(const Color &color);

/** Scale a colour by alpha, for dimming a defined colour without redefining it. */
Color operator*(const Color& color, const double alpha);

class WS2812Driver {
// Made for WS2812B.
public:
	WS2812Driver(const uint16_t ledsNum);
	~WS2812Driver();

	/** @param complementary true when the slot drives CHxN rather than CHx, so
	 *  the frame is started with the TIMEx N variant. Comes straight from
	 *  PwmPinConfig::complementary - the driver never decides this itself. */
	void begin(TIM_HandleTypeDef *timer, const uint32_t channel,
	           bool complementary = false);
	void setPixelColor(const uint32_t& ID, const Color& color);
	void show(void);
	void clear();

	// Called from the DMA transfer-complete ISR (via WS2812_FrameCompleteISR):
	// releases the frame semaphore so the next show() may start a transfer.
	void frameCompleteFromISR();

	// Is this the timer the strip was given? HAL_TIM_PWM_PulseFinishedCallback
	// is shared by every PWM channel that completes a DMA transfer, so the
	// interrupt must be filtered - and this is the only place that knows which
	// handle begin() received, so main.c need not name a timer.
	bool ownsTimer(const TIM_HandleTypeDef *htim) const { return htim == _neoPixTim; }

	// The whole ISR side in one call, instance lookup included: true when `htim`
	// is the strip's timer and the frame was consumed, false when the interrupt
	// belonged to some other PWM channel.
	//
	// STATIC because an ISR has no object. The active instance is registered by
	// begin() into a file-local in WS2812Driver.cpp, and this is what lets that
	// stay file-local: Bridge.cpp's C hook is a one-line forward to here rather
	// than something that needs to see the pointer.
	static bool onFrameCompleteISR(TIM_HandleTypeDef *htim);

	// False if the runtime clock tree does not match WS2812_TIM_CLK_HZ (see
	// the assumptions block above): the strip would get out-of-spec timing.
	bool clockOk() const { return _clockValid; }

	uint16_t* getBuffer() { return _pBuff; }


	uint16_t numPixels(void) const { return _numLeds; }
	Color getPixelColor(const uint8_t ID) const { return _pixels[ID]; }

	bool hasBegun() { return _begun; }

	private:
		// Static storage: sized for WS2812_MAX_LEDS, no dynamic allocation.
		Color    _pixels[WS2812_MAX_LEDS] = {};
		uint16_t _pBuff[WS2812_MAX_BUFFER] = {};

		TIM_HandleTypeDef *_neoPixTim = nullptr;
		uint32_t _timCh = 0;
		bool _complementary = false;   // slot drives CHxN, not CHx
		uint16_t _numLeds;   // Number of LEDs in strip (<= WS2812_MAX_LEDS)
		uint16_t _bufferSize;
		bool _begun = false;
		bool _clockValid = false;

		// Given = no frame in flight. Taken by show() before starting a DMA
		// transfer, given back from the transfer-complete ISR. Static storage.
		SemaphoreHandle_t _frameDone = nullptr;
		StaticSemaphore_t _frameDoneCb{};

};

// A 0-bit the strip would read as a 1 (or as noise) if the rounding lands wrong.
static_assert(ws2812TicksToNs(CCR_B0) >= 220 && ws2812TicksToNs(CCR_B0) <= 380,
	"T0H outside WS2812B spec window");
// Same for a 1-bit: too short reads as 0, too long eats the slot's low tail.
static_assert(ws2812TicksToNs(CCR_B1) >= 580 && ws2812TicksToNs(CCR_B1) <= 1000,
	"T1H outside WS2812B spec window");
// CCR >= ARR is 100% duty: the line never returns low and the bit never ends.
static_assert(CCR_B1 < ARR_PERIOD,
	"1-bit high time must leave a low tail inside the bit period");
// The auto-reload register truncates silently above 16 bits, halving the period.
static_assert(ARR_PERIOD <= 0xFFFFu, "TIM15 ARR is 16-bit");

// A non-integer divider means ARR cannot express 800 kHz and every bit drifts.
static_assert(WS2812_TIM_CLK_HZ % WS2812_BIT_HZ == 0,
	"timer clock must be an integer multiple of 800 kHz or the bit period drifts");

// Too short a tail and the strip never latches, so the frame is simply not shown.
static_assert((uint64_t)RESET_PULSE * 1000000000ULL / WS2812_BIT_HZ >= 50000u,
	"reset tail shorter than the 50 us latch the strip needs");

// NDTR counts the DMA slots in 16 bits; overflow would truncate the frame.
static_assert(WS2812_MAX_BUFFER <= 0xFFFFu,
	"frame + reset tail must fit a 16-bit DMA transfer count (NDTR)");



