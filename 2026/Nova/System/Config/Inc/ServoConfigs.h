#pragma once
#include "PWMDriver.hpp"
#include "Timers.h"
#include "BoardProfile.h"   // profile().led_strip

/**
 * Hardware PWM configs for the four servo outputs.
 * IDs 0-3 map directly to servo[0-3] in ServoThread.
 * Pin/timer/AF from stm32g4xx_hal_msp.c MspPostInit.
 * zero_pulse_us: pulse width in µs that defines the home/zero position. Write
 * it as an angle via angle_to_pulse_us() - that helper is constexpr, so the
 * conversion happens at compile time and the field still holds plain µs.
 */

/** Home angle for every servo, in degrees. 0° is one end of travel, i.e.
 *  PULSE_MIN_US (500 µs) - every channel drives a positional servo, so this is
 *  a commanded position and nothing re-asserts it behind a command. */
static constexpr float SERVO_ZERO_DEG = 0.0f;

/* Proves the map really is compile-time evaluable, and pins it: a static_assert
 * can only use a constant expression, so this fails to build if it ever isn't. */
static_assert(angle_to_pulse_us(SERVO_ZERO_DEG) == PULSE_MIN_US, "servo zero drifted from 0 deg");
static_assert(angle_to_pulse_us(0.0f)   == PULSE_MIN_US, "0 deg must map to PULSE_MIN_US");
static_assert(angle_to_pulse_us(180.0f) == PULSE_MAX_US, "180 deg must map to PULSE_MAX_US");

static const PWMConfig SERVO_0_CFG = {
    .tim            = &htim15,
    .channel        = 2,
    .pin            = TIM_SERVO_1_Pin,
    .port           = TIM_SERVO_1_GPIO_Port,
    .af             = GPIO_AF1_TIM15,  // PB15 → TIM15_CH2
    .complementary  = false,
    .zero_pulse_us  = angle_to_pulse_us(SERVO_ZERO_DEG),
};

static const PWMConfig SERVO_1_CFG = {
    .tim            = &htim15,
    .channel        = 1,
    .pin            = TIM_SERVO_2_Pin,
    .port           = TIM_SERVO_2_GPIO_Port,
    .af             = GPIO_AF1_TIM15,  // PB14 → TIM15_CH1
    .complementary  = false,
    .zero_pulse_us  = angle_to_pulse_us(SERVO_ZERO_DEG),
};

static const PWMConfig SERVO_2_CFG = {
    .tim            = &htim1,
    .channel        = 1,
    .pin            = TIM_SERVO_3_Pin,
    .port           = TIM_SERVO_3_GPIO_Port,
    .af             = GPIO_AF6_TIM1,   // PB13 → TIM1_CH1N (complementary)
    .complementary  = true,
    .zero_pulse_us  = angle_to_pulse_us(SERVO_ZERO_DEG),
};

static const PWMConfig SERVO_3_CFG = {
    .tim            = &htim2,
    .channel        = 4,
    .pin            = TIM_SERVO_4_Pin,
    .port           = TIM_SERVO_4_GPIO_Port,
    .af             = GPIO_AF1_TIM2,   // PB11 → TIM2_CH4
    .complementary  = false,
    .zero_pulse_us  = angle_to_pulse_us(SERVO_ZERO_DEG),
};

/**
 * SERVO_0_CFG and SERVO_1_CFG share TIM15 with the WS2812 strip, and PSC/ARR
 * are per-timer: the strip's 800 kHz bit period and a servo's 50 Hz base cannot
 * both be programmed into one timer, whichever channels they use. So a board
 * row may declare a strip OR servos on those channels, never both.
 *
 * Nothing checks this at runtime, and nothing needs to: ServoThread constructs
 * a channel inert unless the profile declares a device on it, so a strip board
 * that declares no servos never touches TIM15 at all. The only way to break the
 * strip is to write a row claiming both - which is what this rejects.
 *
 * The channel numbers are spelled out because PWMConfig cannot be constexpr (it
 * holds GPIOB, an integer-to-pointer cast), so SERVO_n_CFG.tim is not readable
 * in a constant expression. They live here, under the configs that define them:
 * retime a servo above and this list is in your line of sight.
 */
constexpr uint8_t TIM15_SERVO_CHANNELS[] = { 0, 1 };

constexpr bool noTim15Conflict() {
    for (const BoardProfile& p : PROFILES) {
        if (!p.led_strip) continue;
        for (uint8_t ch : TIM15_SERVO_CHANNELS)
            if (p.servo[ch] != NO_DEVICE) return false;
    }
    return true;
}
static_assert(noTim15Conflict(), "a strip board declares a servo on TIM15 (channels 0-1)");
