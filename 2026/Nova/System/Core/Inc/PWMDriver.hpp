#ifndef PWMDRIVER_HPP_
#define PWMDRIVER_HPP_

#include "main.h"

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
    explicit PWMDriver(const PWMConfig& cfg);
    ~PWMDriver();

    /**
     * @brief Set pulse width directly in microseconds.
     * @param us  Pulse width, typ. 500-2500 µs for a servo.
     */
    void set_pulse_us(uint16_t us);

    /**
     * @brief Set servo angle (0-180°). Mapped to 500-2500 µs.
     */
    void set_angle(float angle);

    /**
     * @brief Drive to the zero/home position defined in PWMConfig.
     */
    void zero();

private:
    PWMConfig cfg_;
    uint32_t  channel_hal_;

    void     enable_gpio_clock() const;
    void     set50Hz();
    void     disable_ccr_preload();
    static uint32_t get_timer_clock(TIM_TypeDef* tim);
};

#endif /* PWMDRIVER_HPP_ */
