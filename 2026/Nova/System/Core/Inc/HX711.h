/**
 * @file hx711.hpp
 * @brief Minimal STM32 HAL driver for HX711 (fixed to channel A, gain 128).
 */

#pragma once
#include "BoardProfile.h"   // ConnPads, PinId
#include <cstdint>

class HX711 {
public:
    /** @param hw  the connector this cell is plugged into; the driver takes its
     *             clock and data pads from there and nothing else. */
    explicit HX711(const ConnPads& hw);

    /// Outcome of one 24-bit read attempt. Each failure points at a different wire:
    enum class ReadResultType : uint8_t {
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
    /// Blocks until data is ready, bounded by timeoutMs (10 SPS chip -> 200 ms default).
    ReadResultType read(volatile int32_t& out, uint32_t timeoutMs = 200);

    /// Average a few samples and use that as offset (tare)
    void tare(uint16_t samples = 10);

    /// Get / set offset used in read()
    int32_t getOffset() const { return _offset; }
    void setOffset(int32_t o) { _offset = o; }


private:
    bool pulseClock() const;

    PinId _dout;   // the connector's data pad
    PinId _sck;    // the connector's clock pad
    int32_t       _offset = 0;
};
