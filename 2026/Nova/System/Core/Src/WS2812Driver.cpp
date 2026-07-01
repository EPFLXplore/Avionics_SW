#include "WS2812Driver.h"

uint32_t colorCode(const Color &c)
{
	return ((uint32_t) c.g << 16) | ((uint32_t) c.r << 8) | (uint32_t) c.b;
}

WS2812Driver::WS2812Driver(const uint16_t ledsNum)
    : numLEDs(ledsNum > WS2812_MAX_LEDS ? WS2812_MAX_LEDS : ledsNum), brightness(0)
{
	bufferSize = BITS_PER_LED * numLEDs;       // <= WS2812_MAX_BUFFER, buffers are static
	for (uint16_t i = 0; i < bufferSize; ++i)
		pBuff[i] = 0;
}

WS2812Driver::~WS2812Driver() {
	// Stop the timer and reset it (guarded: never destroyed in practice).
	if (neoPixTim) {
		HAL_TIM_Base_Stop(neoPixTim);
		__HAL_TIM_SET_COUNTER(neoPixTim, 0);
		__HAL_TIM_CLEAR_FLAG(neoPixTim, TIM_FLAG_UPDATE);
	}
}

void WS2812Driver::begin(TIM_HandleTypeDef *timer, uint32_t channel)
{
	if (!begun)
	{
		neoPixTim = timer;
		timCH = channel;

		for (int i = 0; i < bufferSize; i++)
			pBuff[i] = 0;

		// Making sure that the LED strip is cleared.
		this->clear();

		begun = true;
	}
}

// This function changes dmaBuffer. In order to see changes on the LEDS, use show() to activate the PWM timer.
void WS2812Driver::setPixelColor(const uint32_t& ID, const Color& color, bool updateOG = false)
{
	pixels[ID] = color;
	if (updateOG) pixels_full_b[ID] = color;

	uint32_t code = colorCode(color);

	for  (int i(23); i >=0;  i--)
	{
		// Setting duty cycle via a pointer in the dmaBuffer array.
		pBuff[24*ID + 23-i] = (code >> i) & 0x01 ? CCR_B1 : CCR_B0;
	}
}

void WS2812Driver::presetColors(const Color colors[])
{
	for (int i(0); i < numLEDs; i++)
		setPixelColor(i, colors[i], true);
}

// b is a percentage.
// Transform it to an angle because why not.
void WS2812Driver::setBrightness(uint8_t br)
{
	if (br > BRIGHTNESS_SAFETY_THRESH)
		brightness = BRIGHTNESS_SAFETY_THRESH;
	else
		brightness = br;

	for (int i(0); i < numLEDs; i++)
	{
		pixels[i].r = pixels_full_b[i].r * ((float) brightness / 255.0f);
		pixels[i].g = pixels_full_b[i].g * ((float) brightness / 255.0f);
		pixels[i].b = pixels_full_b[i].b * ((float) brightness / 255.0f);
		setPixelColor(i, pixels[i], false);
	}
}

void WS2812Driver::show()
{
	pBuff[bufferSize - 1] = 0;
	HAL_TIM_PWM_Stop_DMA(neoPixTim, timCH);

	HAL_StatusTypeDef ret = HAL_TIM_PWM_Start_DMA(neoPixTim, timCH,
			(uint32_t*) pBuff, bufferSize);
	(void) ret;

	osDelay(1);
}

// Sets color to black (or blank).
void WS2812Driver::clear()
{
	for (int i(0); i < numLEDs; i++)
	{
		pixels[i].r = 0;
		pixels[i].g = 0;
		pixels[i].b = 0;

		setPixelColor((uint8_t) i, pixels[i], true);
	}

	show();
}
