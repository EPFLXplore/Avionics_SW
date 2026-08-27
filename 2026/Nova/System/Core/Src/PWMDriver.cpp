#include "PWMDriver.h"
#include "Pins.h"

PWMDriver::PWMDriver(const PwmPinConfig& config, uint16_t zeroPulseUs, bool enabled)
    : _config(config), _zeroPulseUs(zeroPulseUs),
      _channelHal(halChannel(config.channel)), _enabled(enabled)
{
    if (!_enabled)
        return;   // inert: another subsystem owns this timer on this board

    // Claim the pad as this timer's alternate function. The driver that actually
    // runs does this, rather than trusting the .ioc: the same pad is a servo
    // output on one board profile and the strip's DMA channel on another, and
    // HAL_GPIO_Init is last-writer-wins.
    configPin(_config.pin, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, idOf(_config.af));

    set50Hz();
    disableCcrPreload();

    TIM_HandleTypeDef* tim = timerOf(_config.timer);
    if (_config.complementary) HAL_TIMEx_PWMN_Start(tim, _channelHal);
    else                   HAL_TIM_PWM_Start(tim, _channelHal);
}

PWMDriver::~PWMDriver()
{
    if (!_enabled)
        return;

    TIM_HandleTypeDef* tim = timerOf(_config.timer);
    if (_config.complementary) HAL_TIMEx_PWMN_Stop(tim, _channelHal);
    else                   HAL_TIM_PWM_Stop(tim, _channelHal);
    deinitPin(_config.pin);
}

/* On STM32G4: TIM1, TIM8, TIM15, TIM16, TIM17, TIM20 are on APB2; TIM2-7 on
 * APB1. If the APBx prescaler is not /1 the timer clock is 2x PCLK. */
uint32_t PWMDriver::timerClock() const
{
    const bool apb2 = (_config.timer == TimerId::Tim1 || _config.timer == TimerId::Tim15);
    uint32_t pclk = apb2 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq();

    RCC_ClkInitTypeDef clk = {0};
    uint32_t latency;
    HAL_RCC_GetClockConfig(&clk, &latency);

    const uint32_t div = apb2 ? clk.APB2CLKDivider : clk.APB1CLKDivider;
    if (div != RCC_HCLK_DIV1) pclk *= 2U;
    return pclk;
}

void PWMDriver::set50Hz()
{
    // Prescale to TIMER_TICK_HZ, then ARR = PWM_ARR -> a 20 ms period, so one
    // timer count is one microsecond and setPulseUs() writes CCR directly.
    TIM_TypeDef* t = timerOf(_config.timer)->Instance;
    t->PSC = (timerClock() / TIMER_TICK_HZ) - 1U;
    t->ARR = PWM_ARR;
    t->EGR = TIM_EGR_UG;   // force an update event so PSC and ARR load now
}

void PWMDriver::disableCcrPreload()
{
    TIM_TypeDef* t = timerOf(_config.timer)->Instance;
    switch (_config.channel) {
        case TimCh::Ch1: t->CCMR1 &= ~TIM_CCMR1_OC1PE; break;
        case TimCh::Ch2: t->CCMR1 &= ~TIM_CCMR1_OC2PE; break;
        case TimCh::Ch3: t->CCMR2 &= ~TIM_CCMR2_OC3PE; break;
        case TimCh::Ch4: t->CCMR2 &= ~TIM_CCMR2_OC4PE; break;
    }
}

void PWMDriver::setPulseUs(uint16_t us)
{
    if (!_enabled)
        return;

    uint32_t pulse = us;
    if (pulse > PWM_ARR) pulse = PWM_ARR;

    TIM_TypeDef* t = timerOf(_config.timer)->Instance;
    switch (_channelHal) {
        case TIM_CHANNEL_1: t->CCR1 = pulse; break;
        case TIM_CHANNEL_2: t->CCR2 = pulse; break;
        case TIM_CHANNEL_3: t->CCR3 = pulse; break;
        case TIM_CHANNEL_4: t->CCR4 = pulse; break;
    }
}

void PWMDriver::setAngle(float angle)
{
    // Clamping and the 0-180 deg -> 500-2500 us map both live in the constexpr
    // helper, so a runtime angle and a configured one resolve identically.
    setPulseUs(angleToPulseUs(angle));
}

void PWMDriver::zero()
{
    setPulseUs(_zeroPulseUs);
}
