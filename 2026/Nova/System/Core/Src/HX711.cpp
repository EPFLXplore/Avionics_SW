/**
 * @file hx711.cpp
 * @brief Minimal STM32 HAL driver for HX711 (fixed to channel A, gain 128).
 */

#include <HX711.h>
#include "cmsis_os2.h"   // osDelay in the bounded ready-wait

namespace {
/* Same if-chain as PWMDriver::enable_gpio_clock(): a port handle is a pointer,
 * not an index, so there is nothing to switch on. */
void enable_gpio_clock(GPIO_TypeDef* port)
{
    if      (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
}
} // namespace

// ---- Constructor -----------------------------------------------------------
HX711::HX711(GPIO_TypeDef* dout_port, uint16_t dout_pin,
             GPIO_TypeDef* sck_port,  uint16_t sck_pin)
    : dout_port_(dout_port),
      dout_pin_(dout_pin),
      sck_port_(sck_port),
      sck_pin_(sck_pin)
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
    enable_gpio_clock(dout_port_);
    enable_gpio_clock(sck_port_);

    // DOUT: input. The chip drives it push-pull while awake but releases it in
    // power-down, so the pull-up is what makes "DOUT went HIGH" mean something
    // in lineTest(), and what stops available() reading a floating low.
    GPIO_InitTypeDef g = {};
    g.Pin   = dout_pin_;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(dout_port_, &g);

    // SCK: output push-pull. Pulses are ~1 us, so LOW speed is ample.
    g.Pin   = sck_pin_;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(sck_port_, &g);

    // Make sure clock is low and give the chip some time
    HAL_GPIO_WritePin(sck_port_, sck_pin_, GPIO_PIN_RESET);
    //HAL_Delay(100); //not compatible with RTOS
}

uint8_t HX711::lineTest()
{
    uint8_t ok = 0;

    // SCK high >60 us forces power-down; a live chip releases DOUT to HIGH
    // (the external/internal pull-up must be able to lift it).
    HAL_GPIO_WritePin(sck_port_, sck_pin_, GPIO_PIN_SET);
    osDelay(1);
    if (HAL_GPIO_ReadPin(sck_port_, sck_pin_) == GPIO_PIN_SET)   ok |= 0x1;
    if (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_SET) ok |= 0x2;

    // Wake + reset (channel A, gain 128). No conversion can be ready yet, so
    // DOUT must still read HIGH immediately after the falling edge.
    HAL_GPIO_WritePin(sck_port_, sck_pin_, GPIO_PIN_RESET);
    if (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_SET) ok |= 0x4;

    return ok;
}

bool HX711::available() const {
    // HX711 pulls DOUT low when data is ready. It needs to be pulled high by pullups in order to not be low without availability
    return (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_RESET);
}

HX711::ReadResult HX711::read(volatile int32_t& out, uint32_t timeout_ms)
{
    // Wait for data ready, bounded. An unpowered chip or an open DOUT line
    // (external pull-up) leaves DOUT high forever - the old unbounded loop
    // hung the calling thread. SCK is low here, so yielding is safe.
    while (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_SET) {
        if (timeout_ms == 0) return ReadResult::Timeout;
        --timeout_ms;
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
    bool released = (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_SET);

    __enable_irq();

    if (!released) return ReadResult::ClockFault;

    // Sign-extend 24-bit value to 32 bits
    if (value & 0x800000U) {
        value |= 0xFF000000U;
    }

    // Apply stored offset (tare)
    out = static_cast<int32_t>(value) - offset_;
    return ReadResult::Ok;
}

void HX711::tare(uint16_t samples)
{
    int64_t sum = 0;
    uint16_t good = 0;
    if (samples == 0) samples = 1;

    for (uint16_t i = 0; i < samples; ++i) {
        int32_t v = 0;
        if (read(v) == ReadResult::Ok) { sum += v; ++good; }
    }

    if (good) offset_ = static_cast<int32_t>(sum / good);
}

// ---- Private helpers -------------------------------------------------------

bool HX711::pulseClock() const
{
    // ~2.7 µs per phase (60 iters ≈ 45 ns each at 144 MHz). Spec needs >0.2 µs
    // high and <60 µs (power-down); the extra width survives degraded edges on
    // marginal lines (cable capacitance / contact resistance) where ~1 µs
    // pulses can fall below the chip's input threshold.
    constexpr int PHASE_LOOPS = 60;

    HAL_GPIO_WritePin(sck_port_, sck_pin_, GPIO_PIN_SET);
    for (volatile int i = 0; i < PHASE_LOOPS; ++i) { __NOP(); }

    bool bit = (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_SET);
    HAL_GPIO_WritePin(sck_port_, sck_pin_, GPIO_PIN_RESET);
    for (volatile int i = 0; i < PHASE_LOOPS; ++i) { __NOP(); }

    return bit;
}
