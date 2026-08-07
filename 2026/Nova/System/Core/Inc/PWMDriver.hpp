#ifndef PWMDRIVER_HPP_
#define PWMDRIVER_HPP_

#include "main.h"

/**
 * Servo travel limits: 0° is kPulseMinUs, kAngleMaxDeg is kPulseMaxUs.
 * These are the single source of truth for the angle<->pulse map; change them
 * and both set_angle() and every angle_to_pulse_us() in ServoConfigs.h follow.
 */
constexpr uint16_t kPulseMinUs  = 500;
constexpr uint16_t kPulseMaxUs  = 2500;
constexpr float    kAngleMaxDeg = 180.0f;

/**
 * @brief Angle in degrees -> pulse width in µs, clamped to 0-kAngleMaxDeg.
 *
 * constexpr so ServoConfigs.h can state home positions as angles rather than
 * hand-computed microseconds, at zero runtime cost. set_angle() calls the same
 * function, so the compile-time and runtime mappings cannot drift apart.
 */
constexpr uint16_t angle_to_pulse_us(float angle)
{
    return angle <= 0.0f         ? kPulseMinUs
         : angle >= kAngleMaxDeg ? kPulseMaxUs
         : static_cast<uint16_t>(kPulseMinUs +
                                 (angle / kAngleMaxDeg) * (kPulseMaxUs - kPulseMinUs));
}

struct PWMConfig {
    TIM_HandleTypeDef* tim;
    uint8_t            channel;        ///< 1-4
    uint16_t           pin;
    GPIO_TypeDef*      port;
    uint8_t            af;
    bool               complementary;  ///< true for CHxN (e.g. TIM1_CH1N)
    uint16_t           zero_pulse_us;  ///< Home/zero position pulse width in µs
};

class PWMDriver {
public:
    /**
     * @param cfg     Pin/timer/AF configuration.
     * @param enabled When false the driver is fully inert: the constructor
     *                touches no GPIO or timer register and every command is a
     *                no-op. Used for servos whose timer belongs to another
     *                subsystem on this board role (e.g. TIM15 servos on the
     *                LED master, where the WS2812 strip owns TIM15).
     */
    explicit PWMDriver(const PWMConfig& cfg, bool enabled = true);
    ~PWMDriver();

    /**
     * @brief Set pulse width directly in microseconds.
     * @param us  Pulse width, typ. 500-2500 µs for a servo.
     */
    void set_pulse_us(uint16_t us);

    /**
     * @brief Set servo angle in degrees. Clamped and mapped by
     *        angle_to_pulse_us() (0-kAngleMaxDeg -> kPulseMinUs-kPulseMaxUs).
     */
    void set_angle(float angle);

    /**
     * @brief Drive to the zero/home position defined in PWMConfig.
     */
    void zero();

private:
    PWMConfig cfg_;
    uint32_t  channel_hal_;
    bool      enabled_;

    void     enable_gpio_clock() const;
    void     set50Hz();
    void     disable_ccr_preload();
    static uint32_t get_timer_clock(TIM_TypeDef* tim);
};

#endif /* PWMDRIVER_HPP_ */
