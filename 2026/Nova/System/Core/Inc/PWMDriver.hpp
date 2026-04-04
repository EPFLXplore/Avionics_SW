#ifndef PWMDRIVER_HPP_
#define PWMDRIVER_HPP_

#include "main.h"

// Define your period based on CubeMX settings (ARR + 1)
#define DEFAULT_TIM_PERIOD 20000

class PWMDriver {
public:
    PWMDriver(TIM_HandleTypeDef* tim, uint8_t channel, uint16_t PWM_Pin, GPIO_TypeDef *PWM_Port);
    ~PWMDriver();

    void set_pwm(float duty_cycle);

private:
    uint32_t channel;
    TIM_HandleTypeDef* tim;
    uint16_t PWM_Pin;
    GPIO_TypeDef *PWM_Port;
    uint32_t period = DEFAULT_TIM_PERIOD;
};

#endif
