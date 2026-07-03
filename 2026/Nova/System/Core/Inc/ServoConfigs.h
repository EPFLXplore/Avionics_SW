#pragma once
#include "PWMDriver.hpp"
#include "Timers.h"
#include "Bridge.h"   // Board_MasterId()

/**
 * Hardware PWM configs for the four servo outputs.
 * IDs 0-3 map directly to servo[0-3] in ServoThread.
 * Pin/timer/AF from stm32g4xx_hal_msp.c MspPostInit.
 * zero_pulse_us: pulse width in µs that defines the home/zero position.
 */

/**
 * Timer-ownership rule: on the LED-master board (id 3) TIM15 belongs to the
 * WS2812 strip driver, which owns the timer's PSC/ARR/DMA. Servos configured
 * on TIM15 (0 and 1 below) must be constructed inert there, or their 50 Hz
 * time-base setup corrupts the LED bitstream. Servos on TIM1/TIM2 always run.
 */
static inline bool servoTimerFree(const PWMConfig& cfg) {
    return !(Board_MasterId() == 3 && cfg.tim == &htim15);
}
static const PWMConfig SERVO_0_CFG = {
    .tim            = &htim15,
    .channel        = 2,
    .pin            = TIM_SERVO_1_Pin,
    .port           = TIM_SERVO_1_GPIO_Port,
    .af             = GPIO_AF1_TIM15,  // PB15 → TIM15_CH2
    .complementary  = false,
    .zero_pulse_us  = 1500,
};

static const PWMConfig SERVO_1_CFG = {
    .tim            = &htim15,
    .channel        = 1,
    .pin            = TIM_SERVO_2_Pin,
    .port           = TIM_SERVO_2_GPIO_Port,
    .af             = GPIO_AF1_TIM15,  // PB14 → TIM15_CH1
    .complementary  = false,
    .zero_pulse_us  = 1500,
};

static const PWMConfig SERVO_2_CFG = {
    .tim            = &htim1,
    .channel        = 1,
    .pin            = TIM_SERVO_3_Pin,
    .port           = TIM_SERVO_3_GPIO_Port,
    .af             = GPIO_AF6_TIM1,   // PB13 → TIM1_CH1N (complementary)
    .complementary  = true,
    .zero_pulse_us  = 1500,
};

static const PWMConfig SERVO_3_CFG = {
    .tim            = &htim2,
    .channel        = 4,
    .pin            = TIM_SERVO_4_Pin,
    .port           = TIM_SERVO_4_GPIO_Port,
    .af             = GPIO_AF1_TIM2,   // PB11 → TIM2_CH4
    .complementary  = false,
    .zero_pulse_us  = 1500,
};
