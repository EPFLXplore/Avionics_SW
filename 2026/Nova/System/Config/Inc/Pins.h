/*
 * Pins.h  --  the one place BoardProfile's enums become HAL calls.
 *
 * BoardProfile.h is deliberately HAL-free so its table can be checked at compile
 * time: GPIOC, I2C3 and htim15 are integer-to-pointer casts, which are not
 * constant expressions, so a table holding them can prove nothing about itself.
 *
 * This header is the other half of that trade. A driver takes PortId / BusId /
 * TimerId from the board table and calls these at the moment it actually has to
 * touch a register - never before. That keeps every if-chain in ONE file instead
 * of one per driver, which is what the three near-identical enableGpioClock()
 * copies used to be.
 *
 * Include this in drivers, not in headers that only describe hardware.
 */

#pragma once

#include "main.h"
#include "BoardProfile.h"  // PortId, BusId, TimerId, PinId, maskOf

#include "tim.h"          // htim1, htim2, htim15 (CubeMX now generates these)

/** GPIO block for a port. A port handle is a pointer, not an index, so there is
 *  nothing to switch on. */
inline GPIO_TypeDef* portOf(PortId p) {
    switch (p) {
        case PortId::A: return GPIOA;
        case PortId::B: return GPIOB;
        case PortId::C: return GPIOC;
        case PortId::D: return GPIOD;
        case PortId::E: return GPIOE;
        case PortId::F: return GPIOF;
        default:        return GPIOG;
    }
}

/** Turn on a port's clock. Idempotent - the HAL macro is a read-modify-write of
 *  a set bit, so a driver may call it without knowing who else has. */
inline void enableGpioClock(PortId p) {
    switch (p) {
        case PortId::A: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case PortId::B: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case PortId::C: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        case PortId::D: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
        case PortId::E: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
        case PortId::F: __HAL_RCC_GPIOF_CLK_ENABLE(); break;
        default:        __HAL_RCC_GPIOG_CLK_ENABLE(); break;
    }
}

/** I2C peripheral behind a connector's pads, or nullptr for BusId::None - which
 *  a caller should never reach, because deviceFitsSlot() refuses to put an I2C
 *  device on a connector whose bus is None. */
inline I2C_TypeDef* busOf(BusId b) {
    return b == BusId::I2c3 ? I2C3 : nullptr;
}

inline void enableBusClock(BusId b) {
    if (b == BusId::I2c3) __HAL_RCC_I2C3_CLK_ENABLE();
}

/** CubeMX's handle for a timer. MX_TIMx_Init initialises them; this only names
 *  them. */
inline TIM_HandleTypeDef* timerOf(TimerId t) {
    switch (t) {
        case TimerId::Tim1:  return &htim1;
        case TimerId::Tim2:  return &htim2;
        case TimerId::Tim15: return &htim15;
        default:             return nullptr;
    }
}

/* ---- pad access, so drivers do not each rewrite it ------------------------ */

inline void writePin(PinId p, GPIO_PinState state) {
    HAL_GPIO_WritePin(portOf(p.port), maskOf(p), state);
}

inline GPIO_PinState readPin(PinId p) {
    return HAL_GPIO_ReadPin(portOf(p.port), maskOf(p));
}

/*
 * TIMINGR for ~95 kHz standard mode from a 144 MHz I2C kernel clock. Fields,
 * per RM0440 I2C_TIMINGR:
 *
 *   PRESC  = 0xB  -> t_presc = 12 / 144 MHz = 83.3 ns
 *   SCLL   = 0x40 -> t_LOW   = 65 * 83.3 ns = 5.42 us  (>= 4.7 us)
 *   SCLH   = 0x3D -> t_HIGH  = 62 * 83.3 ns = 5.17 us  (>= 4.0 us)
 *   SDADEL = 0x2  -> 167 ns data hold
 *   SCLDEL = 0x4  -> 417 ns data setup (>= 250 ns)
 *
 * -> period 10.6 us, ~95 kHz: deliberately under 100 kHz so a long harness to a
 * sensor connector has margin. It lives here rather than in a driver because it
 * is derived from the MCU's clock tree, not from any particular chip.
 */
inline constexpr uint32_t I2C_TIMING_STD_144MHZ = 0xB0423D40;

/**
 * Bring up an I2C bus as a 7-bit controller, into a handle the CALLER owns.
 *
 * The driver owns the init even where CubeMX also generates one for the same
 * peripheral. That is deliberate and matches how every other pad and peripheral
 * here works: what the hardware ends up doing is readable in the driver, not
 * split between the driver and whatever the .ioc last said. This wrapper exists
 * so that stays one line at the call site instead of ten.
 */
inline void initI2c(I2C_HandleTypeDef& h, BusId bus, uint32_t timing) {
    enableBusClock(bus);
    h.Instance              = busOf(bus);
    h.Init.Timing           = timing;
    h.Init.OwnAddress1      = 0;
    h.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    h.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    h.Init.OwnAddress2      = 0;
    h.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    h.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    h.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    (void)HAL_I2C_Init(&h);
}

/** Release a pad back to its reset state. */
inline void deinitPin(PinId p) {
    HAL_GPIO_DeInit(portOf(p.port), maskOf(p));
}

/** Claim a pad. `af` is ignored unless the mode is an alternate function. */
inline void configPin(PinId p, uint32_t mode, uint32_t pull,
                      uint32_t speed = GPIO_SPEED_FREQ_LOW, uint8_t af = 0) {
    enableGpioClock(p.port);
    GPIO_InitTypeDef g = {};
    g.Pin       = maskOf(p);
    g.Mode      = mode;
    g.Pull      = pull;
    g.Speed     = speed;
    g.Alternate = af;
    HAL_GPIO_Init(portOf(p.port), &g);
}

/* ---- what a PWM pad implies ----------------------------------------------- */
/*
 * Fixed by the silicon's pin-mux once the board has chosen the pad: PB15 IS
 * TIM15_CH2 at AF1, PB13 IS TIM1_CH1N. Not in BoardProfile.h, because editing
 * these configures nothing - it only makes a driver wrong.
 *
 * Shared, because a PWM slot can be driven by more than one driver. Pwm1 is a
 * servo output on board 0 and the WS2812 strip's DMA channel on board 3, and
 * both need the same timer and channel - so this cannot live inside either
 * driver's own config without the other having to reach into it.
 */
struct PwmMux {
    PinId   pin;
    TimerId timer;
    uint8_t channel;        ///< 1..4
    uint8_t af;
    bool    complementary;  ///< drive CHxN rather than CHx
};

inline constexpr PwmMux PWM_MUX[PWM_COUNT] = {
    /* Pwm0 */ { pwmPinOf(ConnType::Pwm0), TimerId::Tim15, 2, 1, false },
    /* Pwm1 */ { pwmPinOf(ConnType::Pwm1), TimerId::Tim15, 1, 1, false },
    /* Pwm2 */ { pwmPinOf(ConnType::Pwm2), TimerId::Tim1,  1, 6, true  },  // CH1N
    /* Pwm3 */ { pwmPinOf(ConnType::Pwm3), TimerId::Tim2,  4, 1, false },
};

inline constexpr const PwmMux& pwmMux(ConnType c) { return PWM_MUX[idOf(c) - PWM_FIRST]; }

/** The HAL's channel constant. */
inline constexpr uint32_t halChannel(uint8_t ch) {
    return ch == 1 ? TIM_CHANNEL_1
         : ch == 2 ? TIM_CHANNEL_2
         : ch == 3 ? TIM_CHANNEL_3
         :           TIM_CHANNEL_4;
}

/* What a caller driving a PWM slot actually needs, without unpacking the mux. */
inline TIM_HandleTypeDef* pwmTimerOf(ConnType c)   { return timerOf(pwmMux(c).timer); }
inline uint32_t           pwmChannelOf(ConnType c) { return halChannel(pwmMux(c).channel); }
