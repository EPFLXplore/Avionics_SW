/*
 * WS2812Driver.h
 *
 *  Created on: Feb 28, 2026
 *      Author: Mohamed Gdoura
 *
 *  Bit-banged-over-PWM/DMA driver for WS2812B addressable LEDs.
 *  (Renamed from Adafruit_NeoPixel_STM; statically allocated: no heap.)
 */

#ifndef WS2812_DRIVER_H_
#define WS2812_DRIVER_H_

#include "stm32g4xx_hal.h"
#include <math.h>
#include <cstdint>
#include <array>
#include "cmsis_os2.h"

// Defining constants. Bit timing is for the APB timer clock = 144 MHz:
//   144 MHz / (ARR_PERIOD+1) = 800 kHz  ->  ARR_PERIOD = 179
//   33% duty = CCR_B0, 66% duty = CCR_B1
#define ARR_PERIOD 179
#define CCR_B0 60    // 33% duty cycle.
#define CCR_B1 120   // 66% duty cycle.

#define BRIGHTNESS_SAFETY_THRESH 200 // DO NOT EXCEED TO AVOID XPLOSION.
#define PI 3.14

#define BITS_PER_LED 24
#define RESET_PULSE 50

// Compile-time maximum strip length: sizes the static buffers (no heap).
static constexpr uint16_t WS2812_MAX_LEDS   = 60;
static constexpr uint16_t WS2812_MAX_BUFFER = WS2812_MAX_LEDS * BITS_PER_LED + RESET_PULSE;

// Array containing RGB values. 0 = R | 1 = G | 2 = B.
struct Color {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
};

uint32_t colorCode(const Color &c);

Color operator*(const Color& c, const double alpha);

class WS2812Driver {
// Made for WS2812B.
public:
	WS2812Driver(const uint16_t ledsNum);
	~WS2812Driver();

	void begin(TIM_HandleTypeDef *timer, const uint32_t channel);
	void setPixelColor(const uint32_t& ID, const Color& color, bool updateOG);
	void presetColors(const Color colors[]);
	void setBrightness(uint8_t br);
	void show(void);
	void clear();
	void setBusy(bool isit) { isBusy = isit; }

	uint16_t* getBuffer() { return pBuff; }

	uint8_t getBrightness(void) const { return brightness; }

	uint16_t numPixels(void) const { return numLEDs; }
	Color getPixelColor(const uint8_t ID) const { return pixels[ID]; }

	bool hasBegun() { return begun; }

	private:
		// Static storage: sized for WS2812_MAX_LEDS, no dynamic allocation.
		Color    pixels[WS2812_MAX_LEDS] = {};
		Color    pixels_full_b[WS2812_MAX_LEDS] = {};
		uint16_t pBuff[WS2812_MAX_BUFFER] = {};

		TIM_HandleTypeDef *neoPixTim = nullptr;
		uint32_t timCH = 0;
		uint16_t numLEDs;   // Number of LEDs in strip (<= WS2812_MAX_LEDS)
		uint16_t bufferSize;
		uint8_t brightness = 128; // Strip brightness (0-255)
		bool begun = false;
		bool isBusy = false;

};

#endif
