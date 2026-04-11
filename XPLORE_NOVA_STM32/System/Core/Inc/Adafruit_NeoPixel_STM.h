/*
 * Adafruit_NeoPixel_STM.h
 *
 *  Created on: Feb 28, 2026
 *      Author: Mohamed Gdoura
 */

#ifndef ADAFRUIT_NEOPIXEL_STM_H_
#define ADAFRUIT_NEOPIXEL_STM_H_

#include "stm32h7xx_hal.h"
#include <math.h>
#include <cstdint>
#include <array>
#include <iostream>

using namespace std;

// Defining constants.
// Timer clock = 64 MHz (HSI, APB1×2), ARR = 79 → 80-count period = 1.25 µs @ 800 kHz (WS2812B).
// CCR_B0: ~33% of 80 = 26 counts high (0-bit:  ~0.40 µs high, ~0.85 µs low)
// CCR_B1: ~67% of 80 = 53 counts high (1-bit:  ~0.83 µs high, ~0.42 µs low)
// If you switch to a 144 MHz timer with ARR=179, restore CCR_B0=60, CCR_B1=120.
#define ARR_PERIOD 79
#define CCR_B0 26   // 32.5% duty cycle  (logical 0)
#define CCR_B1 53   // 66.25% duty cycle (logical 1)

#define BRIGHTNESS_SAFETY_THRESH 200 // DO NOT EXCEED TO AVOID XPLOSION.
#define PI 3.14

#define BITS_PER_LED 24
#define RESET_PULSE 50

// Array containing RGB values. 0 = R | 1 = G | 2 = B.
struct Color {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
};

uint32_t colorCode(const Color &c);

Color operator*(const Color& c, const double alpha);

class Adafruit_NeoPixel {
// Made for WS2812B.
public:
	Adafruit_NeoPixel(const uint16_t ledsNum);
	~Adafruit_NeoPixel();

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
		Color* pixels = nullptr;
		Color* pixels_full_b = nullptr;
		uint16_t *pBuff = nullptr;
		TIM_HandleTypeDef *neoPixTim = nullptr;
		uint32_t timCH;
		uint16_t numLEDs;   // Number of LEDs in strip
		uint16_t bufferSize;
		uint8_t brightness = 128; // Strip brightness (0-255)
		bool begun = false;
		bool isBusy = false;

};

#endif
