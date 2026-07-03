#include "WS2812Driver.h"

// The single active driver instance, set by begin(). The DMA transfer-complete
// ISR (HAL_TIM_PWM_PulseFinishedCallback in main.c) reaches it through the C
// hook below; one strip per board, so a single slot is enough.
static WS2812Driver *s_activeDriver = nullptr;

extern "C" void WS2812_FrameCompleteISR(void)
{
	if (s_activeDriver)
		s_activeDriver->frameCompleteFromISR();
}

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

		// Runtime check of assumption 1 (header): the timer kernel clock must
		// be WS2812_TIM_CLK_HZ or every CCR/ARR constant is off. STM32 rule:
		// timer clock = PCLK2 when the APB2 prescaler is /1, else PCLK2 x2.
		const uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
		const uint32_t timClk = (pclk2 == HAL_RCC_GetHCLKFreq()) ? pclk2 : pclk2 * 2u;
		clockValid = (timClk == WS2812_TIM_CLK_HZ);

		// Given = idle; taken while a frame is streaming.
		frameDone = xSemaphoreCreateBinaryStatic(&frameDoneCb);
		xSemaphoreGive(frameDone);
		s_activeDriver = this;

		// Bit period comes from here, not from the CubeMX TIM15 config: the
		// driver owns its timing (see ARR_PERIOD/CCR_B0/CCR_B1 in the header).
		// The prescaler is forced too: PWMDriver (servo configs on other board
		// roles) shares this timer handle and reprograms PSC to a 1 MHz base
		// at construction, which would slow every bit by 144x.
		neoPixTim->Instance->PSC = 0;
		neoPixTim->Init.Prescaler = 0;
		__HAL_TIM_SET_AUTORELOAD(neoPixTim, ARR_PERIOD);
		neoPixTim->Init.Period = ARR_PERIOD;
		// PSC/ARR sit behind shadow registers: latch them now with an update
		// event, then clear the flag it raises.
		neoPixTim->Instance->EGR = TIM_EGR_UG;
		__HAL_TIM_CLEAR_FLAG(neoPixTim, TIM_FLAG_UPDATE);
		__HAL_TIM_SET_COUNTER(neoPixTim, 0);

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
	if (ID >= numLEDs)
		return;

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

// Debug counters (watch in Live Expressions): show() calls, frames skipped
// because the previous one was still streaming, failed DMA starts, last HAL
// status, clock-tree mismatch. ws2812_dma_complete (main.c) counts finished
// transfers.
volatile uint32_t ws2812_show_calls     = 0;
volatile uint32_t ws2812_frames_skipped = 0;
volatile uint32_t ws2812_start_errors   = 0;
volatile uint32_t ws2812_last_status    = 0;
volatile uint32_t ws2812_clock_bad      = 0;

// Non-blocking: starts the DMA transfer and returns immediately (~us of CPU);
// the hardware streams the frame while every thread keeps running. If the
// previous frame is still in flight the call is a no-op (frame skipped).
void WS2812Driver::show()
{
	ws2812_show_calls++;
	if (!clockValid)
		ws2812_clock_bad++;

	if (xSemaphoreTake(frameDone, 0) != pdTRUE) {
		ws2812_frames_skipped++;
		return;
	}

	// The timer free-runs between frames with the CC1 DMA request enabled, so
	// a stale request is pending by the time the next frame starts. Left
	// alone, it fires one transfer the instant the DMA channel is enabled -
	// at a random phase - shifting the whole frame by one slot (bit-shifted,
	// chaotic colors). Drop the request and flag before re-arming.
	__HAL_TIM_DISABLE_DMA(neoPixTim, TIM_DMA_CC1);
	__HAL_TIM_CLEAR_FLAG(neoPixTim, TIM_FLAG_CC1);

	// Length includes the RESET_PULSE zero-duty tail: it holds the line low
	// >50 us after the last bit so the strip latches, and leaves CCR at 0
	// (line idle-low) once the DMA completes.
	HAL_StatusTypeDef ret = HAL_TIM_PWM_Start_DMA(neoPixTim, timCH,
			(uint32_t*) pBuff, bufferSize + RESET_PULSE);
	ws2812_last_status = ret;
	if (ret != HAL_OK) {
		ws2812_start_errors++;
		// Reset HAL TIM/DMA state and free the slot so the next tick retries.
		HAL_TIM_PWM_Stop_DMA(neoPixTim, timCH);
		xSemaphoreGive(frameDone);
	}
}

void WS2812Driver::frameCompleteFromISR()
{
	BaseType_t woken = pdFALSE;
	xSemaphoreGiveFromISR(frameDone, &woken);
	portYIELD_FROM_ISR(woken);
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
