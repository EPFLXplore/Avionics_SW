/**
 * @file hx711.cpp
 * @brief Minimal STM32 HAL driver for HX711 (fixed to channel A, gain 128).
 */

#include <HX711.h>

// ---- Constructor -----------------------------------------------------------
HX711::HX711(GPIO_TypeDef* dout_port, uint16_t dout_pin,
             GPIO_TypeDef* sck_port,  uint16_t sck_pin)
    : dout_port_(dout_port),
      dout_pin_(dout_pin),
      sck_port_(sck_port),
      sck_pin_(sck_pin)
{
    // GPIO modes must be configured in CubeMX:
    // - dout_pin_  : Input (with pull-up)
    // - sck_pin_   : Output, push-pull, low speed
}

// ---- Public API ------------------------------------------------------------

void HX711::begin()
{
    // Make sure clock is low and give the chip some time
    HAL_GPIO_WritePin(sck_port_, sck_pin_, GPIO_PIN_RESET);
    //HAL_Delay(100); //not compatible with RTOS
}

bool HX711::available() const {
    // HX711 pulls DOUT low when data is ready. It needs to be pulled high by pullups in order to not be low without availability
    return (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_RESET);
}

int32_t HX711::read()
{
    // Wait for data ready (RTOS can preempt here safely: SCK is low)
    while (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_SET) {
        // busy wait
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

    __enable_irq();

    // Sign-extend 24-bit value to 32 bits
    if (value & 0x800000U) {
        value |= 0xFF000000U;
    }

    int32_t signedValue = static_cast<int32_t>(value);

    // Apply stored offset (tare)
    return signedValue - offset_;
}

void HX711::tare(uint16_t samples)
{
    int64_t sum = 0;
    if (samples == 0) samples = 1;

    for (uint16_t i = 0; i < samples; ++i) {
        sum += read();   // read() already blocks until ready
    }

    offset_ = static_cast<int32_t>(sum / samples);
}

// ---- Private helpers -------------------------------------------------------

bool HX711::pulseClock() const
{
    HAL_GPIO_WritePin(sck_port_, sck_pin_, GPIO_PIN_SET);
    // HX711 requires >0.2 µs high; sample DOUT while SCK is still HIGH
    for (volatile int i = 0; i < 20; ++i) { __NOP(); }

    bool bit = (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_SET);
    HAL_GPIO_WritePin(sck_port_, sck_pin_, GPIO_PIN_RESET);
    for (volatile int i = 0; i < 20; ++i) { __NOP(); }

    return bit;
}
