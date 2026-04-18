#pragma once
#include "PWMDriver.hpp"
#include "Timers.h"

/**
 * Hardware PWM configs for the four servo outputs.
 * IDs 0-3 map directly to servo[0-3] in ServoThread.
 * Pin/timer/AF from stm32g4xx_hal_msp.c MspPostInit.
 * zero_pulse_us: pulse width in µs that defines the home/zero position.
 */
static const PWMConfig SERVO_0_CFG = {
    .tim            = &htim15,
    .channel        = 2,
//    .pin            = SERVO_TIM1_Pin,
//    .port           = SERVO_TIM1_GPIO_Port,
//    .af             = GPIO_AF4_TIM15,  // TIM15_CH2
    .complementary  = false,
    .zero_pulse_us  = 1500,
};

static const PWMConfig SERVO_1_CFG = {
    .tim            = &htim15,
    .channel        = 1,
//    .pin            = SERVO_TIM2_Pin,
//    .port           = SERVO_TIM2_GPIO_Port,
//    .af             = GPIO_AF4_TIM15,  // TIM15_CH1
    .complementary  = false,
    .zero_pulse_us  = 1500,
};

static const PWMConfig SERVO_2_CFG = {
    .tim            = &htim1,
    .channel        = 1,
//    .pin            = SERVO_TIM3_Pin,
//    .port           = SERVO_TIM3_GPIO_Port,
//    .af             = GPIO_AF1_TIM1,   // TIM1_CH1N (complementary)
    .complementary  = true,
    .zero_pulse_us  = 1500,
};

static const PWMConfig SERVO_3_CFG = {
    .tim            = &htim2,
    .channel        = 4,
//    .pin            = SERVO_TIM4_Pin,
//    .port           = SERVO_TIM4_GPIO_Port,
//    .af             = GPIO_AF1_TIM2,   // TIM2_CH4
    .complementary  = false,
    .zero_pulse_us  = 1500,
};
