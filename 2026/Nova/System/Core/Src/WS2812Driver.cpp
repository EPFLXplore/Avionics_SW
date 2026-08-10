#include "WS2812Driver.h"

// The single active driver instance, set by begin(). The DMA transfer-complete
// ISR (HAL_TIM_PWM_PulseFinishedCallback in main.c) reaches it through the C
// hook below; one strip per board, so a single slot is enough.
static WS2812Driver *s_activeDriver = nullptr;

// Returns 1 when this interrupt belonged to the strip, 0 otherwise. The filter
// lives here rather than in main.c because only the driver knows which handle it
// was given - and that follows LED_STRIP_SLOT, which main.c cannot see.
extern "C" int WS2812_FrameCompleteISR(TIM_HandleTypeDef *htim)
{
	if (!s_activeDriver || !s_activeDriver->ownsTimer(htim))
		return 0;
	s_activeDriver->frameCompleteFromISR();
	return 1;
}

// TIM_CHANNEL_x -> the timer's DMA request enable bit, and its capture/compare
// flag. The HAL constants are not an arithmetic sequence, so they get spelled
// out; keeping the mapping here means the driver works on whichever channel
// LED_STRIP_SLOT resolves to instead of assuming CC1.
static uint32_t dmaRequestOf(uint32_t channel)
{
	return channel == TIM_CHANNEL_1 ? TIM_DMA_CC1
	     : channel == TIM_CHANNEL_2 ? TIM_DMA_CC2
	     : channel == TIM_CHANNEL_3 ? TIM_DMA_CC3
	     :                            TIM_DMA_CC4;
}

static uint32_t ccFlagOf(uint32_t channel)
{
	return channel == TIM_CHANNEL_1 ? TIM_FLAG_CC1
	     : channel == TIM_CHANNEL_2 ? TIM_FLAG_CC2
	     : channel == TIM_CHANNEL_3 ? TIM_FLAG_CC3
	     :                            TIM_FLAG_CC4;
}

uint32_t colorCode(const Color &c)
{
	return ((uint32_t) c.g << 16) | ((uint32_t) c.r << 8) | (uint32_t) c.b;
}

WS2812Driver::WS2812Driver(const uint16_t ledsNum)
    : _numLeds(ledsNum > WS2812_MAX_LEDS ? WS2812_MAX_LEDS : ledsNum), _brightness(0)
{
	_bufferSize = BITS_PER_LED * _numLeds;       // <= WS2812_MAX_BUFFER, buffers are static
	for (uint16_t i = 0; i < _bufferSize; ++i)
		_pBuff[i] = 0;
}

WS2812Driver::~WS2812Driver() {
	// Stop the timer and reset it (guarded: never destroyed in practice).
	if (_neoPixTim) {
		HAL_TIM_Base_Stop(_neoPixTim);
		__HAL_TIM_SET_COUNTER(_neoPixTim, 0);
		__HAL_TIM_CLEAR_FLAG(_neoPixTim, TIM_FLAG_UPDATE);
	}
}

void WS2812Driver::begin(TIM_HandleTypeDef *timer, uint32_t channel, bool complementary)
{
	if (!_begun)
	{
		_neoPixTim = timer;
		_timCh = channel;
		_complementary = complementary;

		// Runtime check of assumption 1 (header): the timer kernel clock must
		// be WS2812_TIM_CLK_HZ or every CCR/ARR constant is off. STM32 rule:
		// timer clock = its APB clock when that prescaler is /1, else APB x2.
		//
		// WHICH APB depends on the timer, so it is read off the handle rather
		// than assumed: this used to call HAL_RCC_GetPCLK2Freq() outright, which
		// is right for TIM1/TIM15 and wrong for a strip on TIM2 or TIM5 - and
		// wrong here means the check passes while the bit period is off.
		const TIM_TypeDef* inst = _neoPixTim->Instance;
		const bool apb2 = (inst == TIM1  || inst == TIM8
#ifdef TIM20
		                || inst == TIM20
#endif
		                || inst == TIM15 || inst == TIM16 || inst == TIM17);
		const uint32_t pclk = apb2 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq();
		const uint32_t timClk = (pclk == HAL_RCC_GetHCLKFreq()) ? pclk : pclk * 2u;
		_clockValid = (timClk == WS2812_TIM_CLK_HZ);

		// Given = idle; taken while a frame is streaming.
		_frameDone = xSemaphoreCreateBinaryStatic(&_frameDoneCb);
		xSemaphoreGive(_frameDone);
		s_activeDriver = this;

		// Bit period comes from here, not from the CubeMX TIM15 config: the
		// driver owns its timing (see ARR_PERIOD/CCR_B0/CCR_B1 in the header).
		// The prescaler is forced too: PWMDriver (servo configs on other board
		// roles) shares this timer handle and reprograms PSC to a 1 MHz base
		// at construction, which would slow every bit by 144x.
		_neoPixTim->Instance->PSC = 0;
		_neoPixTim->Init.Prescaler = 0;
		__HAL_TIM_SET_AUTORELOAD(_neoPixTim, ARR_PERIOD);
		_neoPixTim->Init.Period = ARR_PERIOD;
		// PSC/ARR sit behind shadow registers: latch them now with an update
		// event, then clear the flag it raises.
		_neoPixTim->Instance->EGR = TIM_EGR_UG;
		__HAL_TIM_CLEAR_FLAG(_neoPixTim, TIM_FLAG_UPDATE);
		__HAL_TIM_SET_COUNTER(_neoPixTim, 0);

		for (int i = 0; i < _bufferSize; i++)
			_pBuff[i] = 0;

		// Making sure that the LED strip is cleared.
		this->clear();

		_begun = true;
	}
}

// This function changes dmaBuffer. In order to see changes on the LEDS, use show() to activate the PWM timer.
void WS2812Driver::setPixelColor(const uint32_t& ID, const Color& color, bool updateOG = false)
{
	if (ID >= _numLeds)
		return;

	_pixels[ID] = color;
	if (updateOG) _pixelsFullB[ID] = color;

	uint32_t code = colorCode(color);

	for  (int i(23); i >=0;  i--)
	{
		// Setting duty cycle via a pointer in the dmaBuffer array.
		_pBuff[24*ID + 23-i] = (code >> i) & 0x01 ? CCR_B1 : CCR_B0;
	}
}

void WS2812Driver::presetColors(const Color colors[])
{
	for (int i(0); i < _numLeds; i++)
		setPixelColor(i, colors[i], true);
}

// b is a percentage.
// Transform it to an angle because why not.
void WS2812Driver::setBrightness(uint8_t br)
{
	if (br > BRIGHTNESS_SAFETY_THRESH)
		_brightness = BRIGHTNESS_SAFETY_THRESH;
	else
		_brightness = br;

	for (int i(0); i < _numLeds; i++)
	{
		_pixels[i].r = _pixelsFullB[i].r * ((float) _brightness / 255.0f);
		_pixels[i].g = _pixelsFullB[i].g * ((float) _brightness / 255.0f);
		_pixels[i].b = _pixelsFullB[i].b * ((float) _brightness / 255.0f);
		setPixelColor(i, _pixels[i], false);
	}
}

// Debug counters (watch in Live Expressions): show() calls, frames skipped
// because the previous one was still streaming, failed DMA starts, last HAL
// status, clock-tree mismatch. ws2812DmaComplete (main.c) counts finished
// transfers.
volatile uint32_t ws2812ShowCalls     = 0;
volatile uint32_t ws2812FramesSkipped = 0;
volatile uint32_t ws2812StartErrors   = 0;
volatile uint32_t ws2812LastStatus    = 0;
volatile uint32_t ws2812ClockBad      = 0;

// Non-blocking: starts the DMA transfer and returns immediately (~us of CPU);
// the hardware streams the frame while every thread keeps running. If the
// previous frame is still in flight the call is a no-op (frame skipped).
void WS2812Driver::show()
{
	ws2812ShowCalls++;
	if (!_clockValid)
		ws2812ClockBad++;

	if (xSemaphoreTake(_frameDone, 0) != pdTRUE) {
		ws2812FramesSkipped++;
		return;
	}

	// The timer free-runs between frames with this channel's DMA request
	// enabled, so a stale request is pending by the time the next frame starts.
	// Left alone, it fires one transfer the instant the DMA channel is enabled -
	// at a random phase - shifting the whole frame by one slot (bit-shifted,
	// chaotic colors). Drop the request and flag before re-arming.
	//
	// Derived from _timCh rather than fixed at CC1: which channel the strip runs
	// on is a config choice (LED_STRIP_SLOT plus the .ioc DMA request), and this
	// used to be the one place that silently assumed channel 1.
	__HAL_TIM_DISABLE_DMA(_neoPixTim, dmaRequestOf(_timCh));
	__HAL_TIM_CLEAR_FLAG(_neoPixTim, ccFlagOf(_timCh));

	// Length includes the RESET_PULSE zero-duty tail: it holds the line low
	// >50 us after the last bit so the strip latches, and leaves CCR at 0
	// (line idle-low) once the DMA completes.
	//
	// The N variant when the slot drives CHxN: HAL_TIM_PWM_Start_DMA enables
	// CCxE, which puts the frame on the positive output and leaves the pad this
	// board actually uses idle. Same split PWMDriver makes for a servo on a
	// complementary channel; both are fed from PwmMux::complementary, so the
	// mux stays the single statement of what a slot is.
	HAL_StatusTypeDef ret = _complementary
			? HAL_TIMEx_PWMN_Start_DMA(_neoPixTim, _timCh,
					(uint32_t*) _pBuff, _bufferSize + RESET_PULSE)
			: HAL_TIM_PWM_Start_DMA(_neoPixTim, _timCh,
					(uint32_t*) _pBuff, _bufferSize + RESET_PULSE);
	ws2812LastStatus = ret;
	if (ret != HAL_OK) {
		ws2812StartErrors++;
		// Reset HAL TIM/DMA state and free the slot so the next tick retries.
		if (_complementary) HAL_TIMEx_PWMN_Stop_DMA(_neoPixTim, _timCh);
		else                HAL_TIM_PWM_Stop_DMA(_neoPixTim, _timCh);
		xSemaphoreGive(_frameDone);
	}
}

void WS2812Driver::frameCompleteFromISR()
{
	BaseType_t woken = pdFALSE;
	xSemaphoreGiveFromISR(_frameDone, &woken);
	portYIELD_FROM_ISR(woken);
}

// Sets color to black (or blank).
void WS2812Driver::clear()
{
	for (int i(0); i < _numLeds; i++)
	{
		_pixels[i].r = 0;
		_pixels[i].g = 0;
		_pixels[i].b = 0;

		setPixelColor((uint8_t) i, _pixels[i], true);
	}

	show();
}
