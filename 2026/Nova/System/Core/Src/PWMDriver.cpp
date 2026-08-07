#include "PWMDriver.hpp"

static uint32_t channel_to_hal(uint8_t ch)
{
    switch (ch) {
        case 1:  return TIM_CHANNEL_1;
        case 2:  return TIM_CHANNEL_2;
        case 3:  return TIM_CHANNEL_3;
        case 4:  return TIM_CHANNEL_4;
        default: return TIM_CHANNEL_1;
    }
}

PWMDriver::PWMDriver(const PWMConfig& cfg, bool enabled)
    : cfg_(cfg), channel_hal_(channel_to_hal(cfg.channel)), enabled_(enabled)
{
    if (!enabled_)
        return;   // inert: another subsystem owns this timer on this board

    enable_gpio_clock();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = cfg_.pin;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = cfg_.af;
    HAL_GPIO_Init(cfg_.port, &gpio);

    set50Hz();
    disable_ccr_preload();

    if (cfg_.complementary)
        HAL_TIMEx_PWMN_Start(cfg_.tim, channel_hal_);
    else
        HAL_TIM_PWM_Start(cfg_.tim, channel_hal_);
}

PWMDriver::~PWMDriver()
{
    if (!enabled_)
        return;

    if (cfg_.complementary)
        HAL_TIMEx_PWMN_Stop(cfg_.tim, channel_hal_);
    else
        HAL_TIM_PWM_Stop(cfg_.tim, channel_hal_);
    HAL_GPIO_DeInit(cfg_.port, cfg_.pin);
}

void PWMDriver::set_pulse_us(uint16_t us)
{
    if (!enabled_)
        return;

    // After set50Hz(), timer runs at 1 MHz so 1 count = 1 µs.
    // ARR = 19999, so the value is used directly as the CCR.
    uint32_t pulse = us;
    if (pulse > 19999U) pulse = 19999U;

    switch (channel_hal_) {
        case TIM_CHANNEL_1: cfg_.tim->Instance->CCR1 = pulse; break;
        case TIM_CHANNEL_2: cfg_.tim->Instance->CCR2 = pulse; break;
        case TIM_CHANNEL_3: cfg_.tim->Instance->CCR3 = pulse; break;
        case TIM_CHANNEL_4: cfg_.tim->Instance->CCR4 = pulse; break;
    }
}

void PWMDriver::set_angle(float angle)
{
    // Clamping and the 0-180° -> 500-2500 µs map both live in the constexpr
    // helper, so a runtime angle and a config-file angle resolve identically.
    set_pulse_us(angle_to_pulse_us(angle));
}

void PWMDriver::zero()
{
    set_pulse_us(cfg_.zero_pulse_us);
}

uint32_t PWMDriver::get_timer_clock(TIM_TypeDef* tim)
{
    // On STM32G4: TIM1, TIM8, TIM15, TIM16, TIM17, TIM20 are on APB2.
    // Everything else (TIM2-7) is on APB1.
    bool apb2 = (tim == TIM1  || tim == TIM8  ||
                 tim == TIM15 || tim == TIM16 || tim == TIM17);

    uint32_t pclk = apb2 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq();

    // STM32 rule: if the APBx prescaler is not /1, the timer clock is 2x PCLK.
    RCC_ClkInitTypeDef clk = {0};
    uint32_t latency;
    HAL_RCC_GetClockConfig(&clk, &latency);

    uint32_t div = apb2 ? clk.APB2CLKDivider : clk.APB1CLKDivider;
    if (div != RCC_HCLK_DIV1) pclk *= 2U;

    return pclk;
}

void PWMDriver::set50Hz()
{
    // Prescale to exactly 1 MHz, then ARR = 19999 → 50 Hz (20 ms period).
    uint32_t timer_clk = get_timer_clock(cfg_.tim->Instance);
    uint32_t psc       = (timer_clk / 1000000U) - 1U;

    cfg_.tim->Instance->PSC = psc;
    cfg_.tim->Instance->ARR = 19999U;
    // Force an update event so PSC and ARR are loaded immediately.
    cfg_.tim->Instance->EGR = TIM_EGR_UG;
}

void PWMDriver::disable_ccr_preload()
{
    switch (cfg_.channel) {
        case 1: cfg_.tim->Instance->CCMR1 &= ~TIM_CCMR1_OC1PE; break;
        case 2: cfg_.tim->Instance->CCMR1 &= ~TIM_CCMR1_OC2PE; break;
        case 3: cfg_.tim->Instance->CCMR2 &= ~TIM_CCMR2_OC3PE; break;
        case 4: cfg_.tim->Instance->CCMR2 &= ~TIM_CCMR2_OC4PE; break;
    }
}

void PWMDriver::enable_gpio_clock() const
{
    if      (cfg_.port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (cfg_.port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (cfg_.port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (cfg_.port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (cfg_.port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (cfg_.port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (cfg_.port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
}
