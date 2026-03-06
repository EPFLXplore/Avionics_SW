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
#define ARR_PERIOD 79
#define CCR_B0 27   // 33% duty cycle.
#define CCR_B1 52   // 66% duty cycle.

#define BRIGHTNESS_SAFETY_THRESH 80 // DO NOT EXCEED 80% BRIGHTNESS TO AVOID XPLOSION.
#define PI 3.14

// Array containing RGB values. 0 = R | 1 = G | 2 = B.
struct Color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

uint32_t colorCode(const Color &c);

Color operator*(const Color& c, const double alpha);

class Adafruit_NeoPixel {
// Made for WS2812B.
public:
	Adafruit_NeoPixel(const uint16_t ledsNum);
	~Adafruit_NeoPixel();

	bool begin(TIM_HandleTypeDef *timer, const uint32_t channel);
	void setPixelColor(const uint8_t& ID, const Color color);
	void presetColors(const Color colors[]);
	void setBrightness(uint8_t br);
	void show(void);
	void clear();
	void setBusy(bool isit) { isBusy = isit; }

	uint32_t* getBuffer() { return pBuff; }

	uint8_t getBrightness(void) const { return brightness; }

	uint16_t numPixels(void) const { return numLEDs; }
	Color getPixelColor(const uint8_t ID) const { return pixels[ID]; }

	bool hasBegun() { return begun; }

	private:
		Color* pixels;
		Color* pixels_full_b;
		uint32_t *pBuff;
		TIM_HandleTypeDef *neoPixTim;
		uint32_t timCH;
		uint16_t numLEDs;   // Number of LEDs in strip
		uint8_t brightness; // Strip brightness (percentage)
		bool begun = false;
		bool isBusy = false;

};

#endif
