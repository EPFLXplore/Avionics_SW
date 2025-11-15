/**
 * @file hx711.hpp
 * @brief Minimal STM32 HAL driver for HX711 (fixed to channel A, gain 128).
 */

#pragma once
#include "main.h"
#include <cstdint>

class HX711 {
public:
    /**
     * @param dout_port  GPIO port for DOUT pin (e.g. GPIOB)
     * @param dout_pin   GPIO pin for DOUT pin (e.g. GPIO_PIN_9)
     * @param sck_port   GPIO port for SCK pin
     * @param sck_pin    GPIO pin for SCK pin
     */
    HX711(GPIO_TypeDef* dout_port, uint16_t dout_pin,
          GPIO_TypeDef* sck_port,  uint16_t sck_pin);

    /// Put SCK low and let the chip settle
    void begin();

    /// true when data is ready (DOUT == LOW)
    bool available() const;

    /// Blocking read of one 24-bit sample (signed). Returns raw value minus stored offset.
    int32_t read();

    /// Average a few samples and use that as offset (tare)
    void tare(uint16_t samples = 10);

    /// Get / set offset used in read()
    int32_t getOffset() const { return offset_; }
    void setOffset(int32_t o) { offset_ = o; }

private:
    void pulseClock() const;

    GPIO_TypeDef* dout_port_;
    uint16_t      dout_pin_;
    GPIO_TypeDef* sck_port_;
    uint16_t      sck_pin_;
    int32_t       offset_ = 0;
};
