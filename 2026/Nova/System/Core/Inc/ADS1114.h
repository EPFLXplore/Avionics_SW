/*
 * ADS1114.h  --  TI ADS1114(-Q1) 16-bit delta-sigma ADC over I2C.
 *
 * Datasheet: SBAS563E. Single differential input (AINP = AIN0, AINN = AIN1),
 * internal reference and oscillator, PGA, 8..860 SPS.
 *
 * SINGLE-SHOT ONLY, driven by the caller. The device powers down between
 * conversions (MODE = 1), so nothing runs unless someone asks: startConversion()
 * kicks one, ready() polls the OS bit, readVolts() takes the result. That split
 * is deliberate - the thread owns the waiting, so it can osDelay() between polls
 * instead of the driver blocking a task for a whole conversion period.
 *
 * The driver owns its I2C peripheral and claims its own pins, the same way
 * PWMDriver and HX711 do: the connector these pins belong to is a bit-banged
 * HX711 on one board profile and a real I2C bus on another, and HAL_GPIO_Init is
 * last-writer-wins, so the driver that actually runs is what makes both cases
 * safe. Nothing in the .ioc configures I2C.
 */

#pragma once
#include "main.h"              // I2C_HandleTypeDef
#include "BoardProfile.h"     // ConnPads, BusId

#include <cstdint>

class ADS1114 {
public:
    /* ---- I2C target address (datasheet Table 8-2, ADDR pin strap) ---------- */
    static constexpr uint8_t ADDR_GND = 0x48; // 1001000b
    static constexpr uint8_t ADDR_VDD = 0x49;
    static constexpr uint8_t ADDR_SDA = 0x4A;
    static constexpr uint8_t ADDR_SCL = 0x4B;

    /* Full-scale range = PGA setting (config bits 11:9). The FSR is the
     * DIFFERENTIAL span; the absolute voltage on either pin must still stay
     * within GND..VDD, so a 3V3 part cannot actually reach +-6.144 V. */
    enum class FsrType : uint8_t {
        V6_144 = 0, V4_096 = 1, V2_048 = 2, V1_024 = 3, V0_512 = 4, V0_256 = 5,
    };

    /* Data rate (config bits 7:5). Lower is quieter; the conversion takes
     * 1/rate, which is what conversionMs() reports. */
    enum class RateType : uint8_t {
        SPS8 = 0, SPS16 = 1, SPS32 = 2, SPS64 = 3,
        SPS128 = 4, SPS250 = 5, SPS475 = 6, SPS860 = 7,
    };

    enum class ResultType : uint8_t {
        Ok = 0,      ///< conversion read back cleanly
        BusError,    ///< NACK or I2C error: chip absent, unpowered, or SDA/SCL open
        NotReady,    ///< no conversion finished yet (OS still low) - poll again
    };

    /** Wiring-probe verdict, bitwise (see probe()). 0x7 = all good. */
    static constexpr uint8_t PROBE_ACK    = 0x1; ///< the chip ACKs its address
    static constexpr uint8_t PROBE_CONFIG = 0x2; ///< config register reads back what we wrote
    static constexpr uint8_t PROBE_CONV   = 0x4; ///< a conversion completed inside its budget

    /**
     * @param pads   the connector's two pads, from pinOf(). The board chooses
     *               these; everything below is what they imply.
     * @param bus    the I2C peripheral those pads reach, and
     * @param af     the alternate function that maps them to it - both fixed by
     *               the silicon, so the caller states them beside the chip's own
     *               settings rather than the board table carrying them.
     * @param addr7  7-bit target address, per the ADDR strap.
     */
    ADS1114(const ConnPads& pads, BusId bus, uint8_t af, uint8_t addr7 = ADDR_GND,
            FsrType fsr = FsrType::V4_096, RateType rate = RateType::SPS8);

    /** Enable clocks, claim the pins as open-drain AF, bring up I2C. Call once,
     *  post-HAL, before any conversion. */
    void begin();

    /** One-shot wiring probe: address ACK + config read-back + one conversion.
     *  Returns the PROBE_* bits; debugger-visible, same idea as HX711::lineTest. */
    uint8_t probe();

    /** Kick a single conversion. The chip returns to power-down when it finishes. */
    ResultType startConversion();

    /** True once the conversion started above has completed (config OS bit high).
     *  Poll this from the caller's loop; it costs one 2-byte register read. */
    bool ready();

    /** Raw 16-bit two's-complement code from the conversion register. */
    ResultType readRaw(int16_t& out);

    /** Conversion register scaled to volts using the configured FSR. */
    ResultType readVolts(float& out);

    /** Volts per LSB: FSR / 2^15 (datasheet Table 8-3, +FS = 0x7FFF). */
    float voltsPerLsb() const;

    /** Nominal conversion time in ms for the configured rate, rounded up. The
     *  datasheet allows +-10% on the data rate, so callers should budget more. */
    uint32_t conversionMs() const;

private:
    static constexpr uint8_t REG_CONVERSION = 0x00;
    static constexpr uint8_t REG_CONFIG     = 0x01;

    uint16_t   configWord(bool start) const;
    ResultType writeReg(uint8_t reg, uint16_t value);
    ResultType readReg(uint8_t reg, uint16_t& value);

    I2C_HandleTypeDef _h{};
    ConnPads          _pads;
    BusId             _bus;
    uint8_t           _af;
    uint8_t           _addr7;
    FsrType           _fsr;
    RateType          _rate;
    bool              _begun = false;
};

