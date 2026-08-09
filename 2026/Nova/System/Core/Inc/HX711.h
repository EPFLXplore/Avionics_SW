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

    /// Outcome of one 24-bit read attempt. Each failure points at a different wire:
    enum class ReadResult : uint8_t {
        Ok,         ///< out contains a fresh sample
        Timeout,    ///< DOUT never went low: no conversion (chip unpowered / DOUT line open)
        ClockFault, ///< DOUT still low after the 25th pulse: chip never saw SCK (CLK line open)
    };

    /// Claim the two pins (DOUT input pull-up, SCK output push-pull), enable
    /// their port clocks, and put SCK low. Call once, post-HAL, before any read:
    /// the driver configures its own pins rather than trusting the .ioc, so a
    /// connector whose pins belong to a peripheral on another board profile
    /// cannot leave this one bit-banging an alternate-function pad.
    void begin();

    /// One-shot wiring probe (chip idle; resets it via a power-down cycle).
    /// Bit set = check passed:
    ///   bit0 (0x1) SCK pad reads back HIGH while driven HIGH (not shorted to GND)
    ///   bit1 (0x2) DOUT went HIGH during forced power-down (chip actually sees SCK)
    ///   bit2 (0x4) DOUT still HIGH right after wake (sane reset, no data yet)
    /// 7 = wiring looks good. 1 = chip never reacts to SCK (open SCK line, swapped
    /// SCK/DOUT, or DOUT stuck low). 0 = SCK net shorted / pin misconfigured.
    uint8_t lineTest();

    /// true when data is ready (DOUT == LOW)
    bool available() const;

    /// Read one 24-bit sample (signed, minus stored offset) into out.
    /// Blocks until data is ready, bounded by timeout_ms (10 SPS chip -> 200 ms default).
    ReadResult read(volatile int32_t& out, uint32_t timeout_ms = 200);

    /// Average a few samples and use that as offset (tare)
    void tare(uint16_t samples = 10);

    /// Get / set offset used in read()
    int32_t getOffset() const { return offset_; }
    void setOffset(int32_t o) { offset_ = o; }


private:
    bool pulseClock() const;

    GPIO_TypeDef* dout_port_;
    uint16_t      dout_pin_;
    GPIO_TypeDef* sck_port_;
    uint16_t      sck_pin_;
    int32_t       offset_ = 0;
};
