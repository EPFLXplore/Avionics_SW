/*
 * WS2812Driver.h
 *
 *  Created on: Feb 28, 2026
 *      Author: Mohamed Gdoura
 *
 *  Bit-banged-over-PWM/DMA driver for WS2812B addressable LEDs.
 *  (Renamed from Adafruit_NeoPixel_STM; statically allocated: no heap.)
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

inline constexpr uint32_t ws2812NsToTicks(uint32_t ns) {
	return (uint32_t)(((uint64_t)WS2812_TIM_CLK_HZ * ns + 500000000ULL) / 1000000000ULL);
}
inline constexpr uint32_t ws2812TicksToNs(uint32_t ticks) {
	return (uint32_t)(((uint64_t)ticks * 1000000000ULL) / WS2812_TIM_CLK_HZ);
}

static_assert(WS2812_TIM_CLK_HZ % WS2812_BIT_HZ == 0,
	"timer clock must be an integer multiple of 800 kHz or the bit period drifts");

inline constexpr uint32_t ARR_PERIOD = WS2812_TIM_CLK_HZ / WS2812_BIT_HZ - 1; // 179 @ 144 MHz
inline constexpr uint32_t CCR_B0 = ws2812NsToTicks(350); // 0-bit high time (50 @ 144 MHz)
inline constexpr uint32_t CCR_B1 = ws2812NsToTicks(700); // 1-bit high time (101 @ 144 MHz)

static_assert(ws2812TicksToNs(CCR_B0) >= 220 && ws2812TicksToNs(CCR_B0) <= 380,
	"T0H outside WS2812B spec window");
static_assert(ws2812TicksToNs(CCR_B1) >= 580 && ws2812TicksToNs(CCR_B1) <= 1000,
	"T1H outside WS2812B spec window");
static_assert(CCR_B1 < ARR_PERIOD,
	"1-bit high time must leave a low tail inside the bit period");
static_assert(ARR_PERIOD <= 0xFFFFu, "TIM15 ARR is 16-bit");

inline constexpr uint16_t BITS_PER_LED = 24;
inline constexpr uint16_t RESET_PULSE  = 50; // zero-duty tail slots appended to each frame

static_assert((uint64_t)RESET_PULSE * 1000000000ULL / WS2812_BIT_HZ >= 50000u,
	"reset tail shorter than the 50 us latch the strip needs");

// Strip length: sizes the static buffers (no heap) and is what the code drives.
inline constexpr uint16_t WS2812_MAX_LEDS   = 72;
inline constexpr uint16_t WS2812_MAX_BUFFER = WS2812_MAX_LEDS * BITS_PER_LED + RESET_PULSE;

static_assert(WS2812_MAX_BUFFER <= 0xFFFFu,
	"frame + reset tail must fit a 16-bit DMA transfer count (NDTR)");

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
inline constexpr uint16_t pixelLoad(const Color& c) { return c.r + c.g + c.b; }

uint32_t colorCode(const Color &c);

Color operator*(const Color& c, const double alpha);

class WS2812Driver {
// Made for WS2812B.
public:
	WS2812Driver(const uint16_t ledsNum);
	~WS2812Driver();

	/** @param complementary true when the slot drives CHxN rather than CHx, so
	 *  the frame is started with the TIMEx N variant. Comes straight from
	 *  PwmMux::complementary - the driver never decides this itself. */
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

// C hook for HAL_TIM_PWM_PulseFinishedCallback (main.c): forwards the DMA
// transfer-complete event to the active driver instance. Returns 1 when the
// interrupt was the strip's, 0 when it belonged to some other PWM channel, so
// the caller can filter without knowing which timer the strip runs on.
extern "C" int WS2812_FrameCompleteISR(TIM_HandleTypeDef *htim);

