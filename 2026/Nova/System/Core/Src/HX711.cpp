/**
 * @file hx711.cpp
 * @brief Minimal STM32 HAL driver for HX711 (fixed to channel A, gain 128).
 */

#include <HX711.h>
#include "Pins.h"
#include "cmsis_os2.h"   // osDelay in the bounded ready-wait

// ---- Constructor -----------------------------------------------------------
HX711::HX711(const ConnPads& hw)
    : _dout(hw.data),
      _sck(hw.clk)
{
    // Pin modes are NOT set here: the constructor runs before HAL is guaranteed
    // ready in some paths, and begin() is the driver's post-HAL entry point.
}

// ---- Public API ------------------------------------------------------------

void HX711::begin()
{
    // Own the pins rather than trusting CubeMX to have configured them.
    //
    // A connector's pins can be plain GPIO for this bit-banged driver or handed
    // to a real peripheral (AF) on a board that uses the connector differently,
    // and MX_GPIO_Init() only sets whatever the .ioc last said they were. Since
    // HAL_GPIO_Init is last-writer-wins, the driver that actually runs claiming
    // its own pins is what makes the two cases safe - the same thing PWMDriver
    // already does for its AF pin.
    // DOUT: input. The chip drives it push-pull while awake but releases it in
    // power-down, so the pull-up is what makes "DOUT went HIGH" mean something
    // in lineTest(), and what stops available() reading a floating low.
    configPin(_dout, GPIO_MODE_INPUT,    GPIO_PULLUP);
    configPin(_sck,  GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);

    // Make sure clock is low and give the chip some time
    writePin(_sck, GPIO_PIN_RESET);
    //HAL_Delay(100); //not compatible with RTOS
}

uint8_t HX711::lineTest()
{
    uint8_t ok = 0;

    // SCK high >60 us forces power-down; a live chip releases DOUT to HIGH
    // (the external/internal pull-up must be able to lift it).
    writePin(_sck, GPIO_PIN_SET);
    osDelay(1);
    if (readPin(_sck) == GPIO_PIN_SET)   ok |= 0x1;
    if (readPin(_dout) == GPIO_PIN_SET) ok |= 0x2;

    // Wake + reset (channel A, gain 128). No conversion can be ready yet, so
    // DOUT must still read HIGH immediately after the falling edge.
    writePin(_sck, GPIO_PIN_RESET);
    if (readPin(_dout) == GPIO_PIN_SET) ok |= 0x4;

    return ok;
}

bool HX711::available() const {
    // HX711 pulls DOUT low when data is ready. It needs to be pulled high by pullups in order to not be low without availability
    return (readPin(_dout) == GPIO_PIN_RESET);
}

HX711::ReadResultType HX711::read(volatile int32_t& out, uint32_t timeoutMs)
{
    // Wait for data ready, bounded. An unpowered chip or an open DOUT line
    // (external pull-up) leaves DOUT high forever - the old unbounded loop
    // hung the calling thread. SCK is low here, so yielding is safe.
    while (readPin(_dout) == GPIO_PIN_SET) {
        if (timeoutMs == 0) return ReadResultType::Timeout;
        --timeoutMs;
        osDelay(1);
    }

    // Disable interrupts for the entire bit-bang sequence.
    // If the RTOS preempts while SCK is HIGH, the HX711 sees SCK high >60 µs
    // and enters power-down mode, causing DOUT to stick HIGH permanently.
    __disable_irq();

    uint32_t value = 0;

    // Read 24 bits, MSB first
    for (int i = 0; i < 24; ++i) {
        value <<= 1;
        if (pulseClock()) value++;
    }

    // One extra pulse: channel A, gain 128
    pulseClock();

    // After the 25th pulse the chip releases DOUT back HIGH (datasheet). If it
    // is still low, the chip never saw our clock train (open SCK line): the 24
    // "bits" above were just a stuck-low pin, not data - reject the sample.
    bool released = (readPin(_dout) == GPIO_PIN_SET);

    __enable_irq();

    if (!released) return ReadResultType::ClockFault;

    // Sign-extend 24-bit value to 32 bits
    if (value & 0x800000U) {
        value |= 0xFF000000U;
    }

    // Apply stored offset (tare)
    out = static_cast<int32_t>(value) - _offset;
    return ReadResultType::Ok;
}

void HX711::tare(uint16_t samples)
{
    int64_t sum = 0;
    uint16_t good = 0;
    if (samples == 0) samples = 1;

    for (uint16_t i = 0; i < samples; ++i) {
        int32_t v = 0;
        if (read(v) == ReadResultType::Ok) { sum += v; ++good; }
    }

    if (good) _offset = static_cast<int32_t>(sum / good);
}

// ---- Private helpers -------------------------------------------------------

bool HX711::pulseClock() const
{
    // ~2.7 µs per phase (60 iters ≈ 45 ns each at 144 MHz). Spec needs >0.2 µs
    // high and <60 µs (power-down); the extra width survives degraded edges on
    // marginal lines (cable capacitance / contact resistance) where ~1 µs
    // pulses can fall below the chip's input threshold.
    constexpr int PHASE_LOOPS = 60;

    writePin(_sck, GPIO_PIN_SET);
    for (volatile int i = 0; i < PHASE_LOOPS; ++i) { __NOP(); }

    bool bit = (readPin(_dout) == GPIO_PIN_SET);
    writePin(_sck, GPIO_PIN_RESET);
    for (volatile int i = 0; i < PHASE_LOOPS; ++i) { __NOP(); }

    return bit;
}
