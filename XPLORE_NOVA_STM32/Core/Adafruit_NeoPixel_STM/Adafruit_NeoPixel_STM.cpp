#include "Adafruit_NeoPixel_STM.h"

uint32_t colorCode(const Color &c)
{
	return ((uint32_t) c.g << 16) | ((uint32_t) c.r << 8) | (uint32_t) c.b;
}

Color operator*(const Color& c, const double alpha)
{
	return {(uint8_t) floor(c.r*alpha), (uint8_t) floor(c.g*alpha), (uint8_t) floor(c.b*alpha)};
}

Adafruit_NeoPixel::Adafruit_NeoPixel(const uint16_t ledsNum)
    : numLEDs(ledsNum), brightness(0)
{
	pBuff = new uint32_t[24*ledsNum];
	pixels = new Color[ledsNum];
	pixels_full_b = new Color[ledsNum];
}

Adafruit_NeoPixel::~Adafruit_NeoPixel() {
	// Stop the timer and reset it.
	HAL_TIM_Base_Stop(neoPixTim);
	__HAL_TIM_SET_COUNTER(neoPixTim, 0);
	__HAL_TIM_CLEAR_FLAG(neoPixTim, TIM_FLAG_UPDATE);
}

bool Adafruit_NeoPixel::begin(TIM_HandleTypeDef *timer, uint32_t channel)
{
	if (!begun)
	{
		// Making sure that the LED strip is cleared.
		this->clear();

		neoPixTim = timer;
		timCH = channel;

		//HAL_TIM_PWM_Start(neoPixTim, channel);
		begun = true;
		return true;
	}
}

// This function changes dmaBuffer. In order to see changes on the LEDS, use show() to activate the PWM timer.
void Adafruit_NeoPixel::setPixelColor(const uint8_t& ID, const Color color)
{
	pixels[ID] = color;
	uint32_t code = colorCode(color);

	for  (int i(23); i >=0;  i--)
	{
		// Setting duty cycle via a pointer in the dmaBuffer array.
		*pBuff = (code >> i) & 0x01 ? CCR_B1 : CCR_B0;
		pBuff++;
	}
}

void Adafruit_NeoPixel::presetColors(const Color colors[])
{
	for (int i(0); i < numLEDs; i++)
	{
		setPixelColor(i, colors[i]);
		pixels_full_b[i] = pixels[i];
	}
}

// b is a percentage.
// Transform it to an angle because why not.
void Adafruit_NeoPixel::setBrightness(uint8_t br)
{
	if (br > BRIGHTNESS_SAFETY_THRESH)
		brightness = BRIGHTNESS_SAFETY_THRESH - 50;
	else
		brightness = br;

	for (int i(0); i < numLEDs; i++)
	{
		pixels[i].r = pixels_full_b[i].r * (brightness / 100);
		pixels[i].g = pixels_full_b[i].g * (brightness / 100);
		pixels[i].b = pixels_full_b[i].b * (brightness / 100);
		setPixelColor(i, pixels[i]);
	}
}

void Adafruit_NeoPixel::show()
{
	HAL_TIM_PWM_Start_DMA(neoPixTim, timCH, pBuff, 24*numLEDs);
	HAL_Delay(10);
}

// Sets color to black (or blank).
void Adafruit_NeoPixel::clear()
{
	// Q: Why the value 1?
	// A: Somewhere on the ESP32 Neopixel library, I saw in setBrightness() that 0 corresponds to full brightness.
	// Risk should be avoided.
	for (int i(0); i < numLEDs; i++)
	{
		pixels[i].r = 1;
		pixels[i].g = 1;
		pixels[i].b = 1;

		setPixelColor((uint8_t) i, pixels[i]);
	}

	show();
}
