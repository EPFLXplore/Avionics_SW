/*
 * ADS1114.cpp  --  see ADS1114.h.
 */

#include <ADS1114.h>
#include "Pins.h"

namespace {
/* I2C blocking-call budget. Every transfer here is 2-3 bytes, so at ~95 kHz a
 * whole exchange is well under 1 ms; 10 ms only ever expires on a dead bus. */
constexpr uint32_t I2C_TIMEOUT_MS = 10;

} // namespace

ADS1114::ADS1114(const ConnPads& pads, BusId bus, uint8_t af,
                 uint8_t addr7, FsrType fsr, RateType rate)
    : _pads(pads),
      _bus(bus),
      _af(af),
      _addr7(addr7),
      _fsr(fsr),
      _rate(rate)
{
    // Nothing touched here: HAL may not be up yet on every path, and begin() is
    // the driver's post-HAL entry point (same contract as HX711).
}

void ADS1114::begin()
{
    if (_begun) return;

    /* Both lines open-drain with pull-ups: I2C devices only ever pull low, and
     * the bus idles high. The internal pull-ups are weak (~40k) and are a
     * fallback only - the board should carry 1k-10k externally (datasheet
     * section 9.1.1), or the rise time will not meet spec on a long harness. */
    configPin(_pads.clk,  GPIO_MODE_AF_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, _af);
    configPin(_pads.data, GPIO_MODE_AF_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, _af);

    initI2c(_h, _bus, I2C_TIMING_STD_144MHZ);

    _begun = true;

    /* Park the chip in a known state: our FSR/rate, single-shot, comparator off
     * and ALERT/RDY high-Z. Without this the device sits at its reset default
     * (0x8583: +-2.048 V, 128 SPS) and the first readVolts() would scale by the
     * wrong FSR. Failure is not fatal here - probe() reports it. */
    (void)writeReg(REG_CONFIG, configWord(false));
}

/* ---- register access ------------------------------------------------------ */

/* Datasheet 8.5.3 / Figure 8-10: address byte, pointer byte, MSB, LSB. */
ADS1114::ResultType ADS1114::writeReg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = { reg,
                       static_cast<uint8_t>(value >> 8),
                       static_cast<uint8_t>(value & 0xFF) };
    const HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(
        &_h, static_cast<uint16_t>(_addr7 << 1), buf, sizeof buf, I2C_TIMEOUT_MS);
    return (st == HAL_OK) ? ResultType::Ok : ResultType::BusError;
}

/* Datasheet 8.5.3 / Figure 8-9: write the pointer, repeated START, read 2 bytes
 * MSB first. HAL_I2C_Mem_Read issues exactly that sequence. */
ADS1114::ResultType ADS1114::readReg(uint8_t reg, uint16_t& value)
{
    uint8_t buf[2] = { 0, 0 };
    const HAL_StatusTypeDef st = HAL_I2C_Mem_Read(
        &_h, static_cast<uint16_t>(_addr7 << 1), reg, I2C_MEMADD_SIZE_8BIT,
        buf, sizeof buf, I2C_TIMEOUT_MS);
    if (st != HAL_OK) return ResultType::BusError;
    value = static_cast<uint16_t>((static_cast<uint16_t>(buf[0]) << 8) | buf[1]);
    return ResultType::Ok;
}

/*
 * Config register, ADS1114 (datasheet Figure 8-16 / Table 8-6):
 *
 *   15    OS        1 = start a single conversion (write, from power-down)
 *   14:12 RESERVED  ADS1114 has no MUX - it is always AINP=AIN0, AINN=AIN1
 *   11:9  PGA       full-scale range
 *   8     MODE      1 = single-shot / power-down
 *   7:5   DR        data rate
 *   4     COMP_MODE 0 = traditional comparator
 *   3     COMP_POL  0 = ALERT/RDY active low
 *   2     COMP_LAT  0 = non-latching
 *   1:0   COMP_QUE  11 = comparator disabled, ALERT/RDY high-Z
 *
 * The comparator is disabled because nothing on this board wires ALERT/RDY;
 * leaving it enabled would drive an open-drain pin nobody reads.
 */
uint16_t ADS1114::configWord(bool start) const
{
    uint16_t cfg = 0;
    if (start) cfg |= (1u << 15);                                  // OS
    cfg |= static_cast<uint16_t>(static_cast<uint8_t>(_fsr))  << 9; // PGA
    cfg |= (1u << 8);                                              // MODE: single-shot
    cfg |= static_cast<uint16_t>(static_cast<uint8_t>(_rate)) << 5;  // DR
    cfg |= 0x3;                                                    // COMP_QUE: disabled
    return cfg;
}

/* ---- public API ----------------------------------------------------------- */

ADS1114::ResultType ADS1114::startConversion()
{
    return writeReg(REG_CONFIG, configWord(true));
}

bool ADS1114::ready()
{
    uint16_t cfg = 0;
    if (readReg(REG_CONFIG, cfg) != ResultType::Ok) return false;
    // OS reads 0 while a conversion is running, 1 when the device is idle.
    return (cfg & (1u << 15)) != 0;
}

ADS1114::ResultType ADS1114::readRaw(int16_t& out)
{
    uint16_t raw = 0;
    const ResultType r = readReg(REG_CONVERSION, raw);
    if (r != ResultType::Ok) return r;
    out = static_cast<int16_t>(raw); // 16-bit two's complement, no sign extension needed
    return ResultType::Ok;
}

float ADS1114::voltsPerLsb() const
{
    // FSR / 2^15: +FS maps to 0x7FFF, -FS to 0x8000 (datasheet Table 8-3).
    switch (_fsr) {
        case FsrType::V6_144: return 6.144f / 32768.0f;
        case FsrType::V4_096: return 4.096f / 32768.0f;
        case FsrType::V2_048: return 2.048f / 32768.0f;
        case FsrType::V1_024: return 1.024f / 32768.0f;
        case FsrType::V0_512: return 0.512f / 32768.0f;
        default:              return 0.256f / 32768.0f;
    }
}

ADS1114::ResultType ADS1114::readVolts(float& out)
{
    int16_t raw = 0;
    const ResultType r = readRaw(raw);
    if (r != ResultType::Ok) return r;
    out = static_cast<float>(raw) * voltsPerLsb();
    return ResultType::Ok;
}

uint32_t ADS1114::conversionMs() const
{
    // 1/rate, rounded up to whole ms. 8 SPS -> 125 ms, 860 SPS -> 2 ms.
    switch (_rate) {
        case RateType::SPS8:   return 125;
        case RateType::SPS16:  return 63;
        case RateType::SPS32:  return 32;
        case RateType::SPS64:  return 16;
        case RateType::SPS128: return 8;
        case RateType::SPS250: return 4;
        case RateType::SPS475: return 3;
        default:           return 2;
    }
}

uint8_t ADS1114::probe()
{
    uint8_t ok = 0;

    // 1. Does anything ACK this address at all? Separates "no chip / no power /
    //    open SDA" from "chip there but misconfigured".
    if (HAL_I2C_IsDeviceReady(&_h, static_cast<uint16_t>(_addr7 << 1), 2,
                              I2C_TIMEOUT_MS) == HAL_OK)
        ok |= PROBE_ACK;

    // 2. Read back what begin() wrote. The OS bit reads as status rather than
    //    the 0 we wrote, so compare everything below it.
    const uint16_t want = configWord(false) & 0x7FFF;
    uint16_t got = 0;
    if (readReg(REG_CONFIG, got) == ResultType::Ok && (got & 0x7FFF) == want)
        ok |= PROBE_CONFIG;

    // 3. One real conversion inside a generous budget. Catches a chip that
    //    talks but never converts (bad reference, brown-out).
    if (startConversion() == ResultType::Ok) {
        const uint32_t budget = conversionMs() * 2 + 5;
        const uint32_t start  = HAL_GetTick();
        while ((HAL_GetTick() - start) < budget) {
            if (ready()) { ok |= PROBE_CONV; break; }
        }
    }

    return ok;
}
