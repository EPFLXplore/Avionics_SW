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
    // HX711 pulls DOUT low when data is ready
    return (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_RESET);
}

int32_t HX711::read()
{
    while (HAL_GPIO_ReadPin(dout_port_,dout_pin_) == GPIO_PIN_SET);
    uint32_t value = 0;
    for (int i = 0; i < 24; ++i) {
            value <<=1;
            if(pulseClock()) {
            	value++;

            }
    }
    pulseClock();

    if(value & 0x800000) {
    	value |= 0xFF000000;
    }

    return static_cast<int32_t>(value);
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
    // short delay: HX711 requires >0.2 µs high
    for (volatile int i = 0; i < 20; ++i) { __NOP(); }

    bool bit = (HAL_GPIO_ReadPin(dout_port_, dout_pin_) == GPIO_PIN_SET);
    HAL_GPIO_WritePin(sck_port_, sck_pin_, GPIO_PIN_RESET);
    for (volatile int i = 0; i < 20; ++i) { __NOP(); }

    return bit;
}
