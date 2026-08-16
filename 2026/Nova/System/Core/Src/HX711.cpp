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
    // power-down, so the pull-up is what stops available() from reading a
    // floating low.
    configPin(_dout, GPIO_MODE_INPUT,    GPIO_PULLUP);
    configPin(_sck,  GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);

    // Make sure clock is low and give the chip some time
    writePin(_sck, GPIO_PIN_RESET);
    //HAL_Delay(100); //not compatible with RTOS
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

    // Interrupt masking lives in pulseClock(), around the HIGH phase only - see
    // the note there. It used to wrap this whole loop.
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
    //
    // Read it immediately: DOUT stays high until the NEXT conversion completes
    // (12.5 ms at 80 SPS), so only a preemption longer than that could see it
    // low again and report a false ClockFault.
    bool released = (readPin(_dout) == GPIO_PIN_SET);

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

    // Interrupts are masked for the HIGH phase ONLY.
    //
    // What needs guarding is being preempted with SCK high: past 60 us the chip
    // powers down (Fig 3) and DOUT sticks high. Being preempted with SCK LOW is
    // harmless - low is the chip's normal idle state, and the bounded ready-wait
    // in read() already yields in exactly that state.
    //
    // This mask used to wrap the whole 25-pulse train, holding interrupts off for
    // ~135 us at a stretch. At 10 SPS that ran 20x/s and nobody noticed; at 80
    // SPS it runs 160x/s, and the USB CDC ISR latency it caused was enough to
    // trip Nexus's 3 s stall detector (STALL_TIMEOUT -> close/reopen -> the
    // calibration replay). Per-pulse masking keeps the pulse width exactly as it
    // was - no edge margin given up on a marginal harness - while cutting
    // worst-case ISR latency from ~135 us to ~2.7 us and halving total masked
    // time, since only the high phases are covered.
    //
    // The cost: the train can now be stretched by preemption, so it must still
    // finish inside one conversion period - 12.5 ms at 80 SPS - or the chip sees
    // the wrong pulse count (datasheet: 25..27 pulses per conversion period).
    // Comms is osPriorityHigh on a 1 ms poll, so preemptions here are tens of us
    // against 12.5 ms of budget.
    //
    // Re-enabled unconditionally rather than save/restore. This is the ONLY
    // place in the firmware that touches PRIMASK - FreeRTOS critical sections
    // use BASEPRI on Cortex-M4, not PRIMASK - so it is always entered with
    // interrupts on and there is nothing to nest with. Forcing them back on is
    // also the safer of the two: a save/restore that ever captured a 1 would
    // latch interrupts off permanently, while this always recovers.
    __disable_irq();

    writePin(_sck, GPIO_PIN_SET);
    for (volatile int i = 0; i < PHASE_LOOPS; ++i) { __NOP(); }

    bool bit = (readPin(_dout) == GPIO_PIN_SET);
    writePin(_sck, GPIO_PIN_RESET);

    __enable_irq();

    // Low phase runs unmasked: SCK low is safe indefinitely.
    for (volatile int i = 0; i < PHASE_LOOPS; ++i) { __NOP(); }

    return bit;
}
