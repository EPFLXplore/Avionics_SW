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

// THIS FILE TECHNICALLY DUPLICATES THE IOC CONFIG BUT WITH ENUM CLASSES SO HARDCODED ASSSUMPTIONS COULD BE CHECKED WHEN COMPILING

#pragma once

#include "main.h"
#include "BoardProfile.h"  // ConnType, the slot counts, PROFILES

#include "tim.h"          // htim1, htim2, htim15 (CubeMX now generates these)

/* ---- pad vocabulary ------------------------------------------------------- */

enum class PortId : uint8_t { A = 0, B, C, D, E, F, G };

/** A pad, as pure data. `bit` is 0..15; maskOf() turns it into a HAL pin mask. */
struct PinId {
    PortId  port;
    uint8_t bit;
};

constexpr uint16_t maskOf(PinId pin)        { return static_cast<uint16_t>(1u << pin.bit); }
constexpr bool samePin(PinId a, PinId b)    { return a.port == b.port && a.bit == b.bit; }

/** Peripherals reachable from a slot's pads. Resolved by Pins.h. */
/* Enumerators must not collide with the CMSIS peripheral macros: I2C3 is
 * already #defined as ((I2C_TypeDef*)I2C3_BASE). Hence I2c3. */
enum class BusId   : uint8_t { None = 0, I2c3 };
enum class TimerId : uint8_t { None = 0, Tim1, Tim2, Tim15 };

/**
 * Every pad this firmware drives, named for the pad itself.
 *
 * Not for whatever is plugged into it: PC8 is an HX711 clock on one board and an
 * I2C clock on another, so a name that picks a side is wrong half the time. This
 * is why the .ioc labels (HX711_CLK_Pin and friends) are not used here.
 */
namespace pin {
inline constexpr PinId PA2  = { PortId::A,  2 };
inline constexpr PinId PA3  = { PortId::A,  3 };
inline constexpr PinId PB11 = { PortId::B, 11 };
inline constexpr PinId PB13 = { PortId::B, 13 };
inline constexpr PinId PB14 = { PortId::B, 14 };
inline constexpr PinId PB15 = { PortId::B, 15 };
inline constexpr PinId PC8  = { PortId::C,  8 };
inline constexpr PinId PC9  = { PortId::C,  9 };
} // namespace pin

/* ---- which pads each slot uses -------------------------------------------- */
/*
 * A CHOICE - the only one on this page. These lines ARE the PCB: change one and
 * you are saying the board changed. Nothing else in the firmware says that.
 *
 * Which is the whole reason the layer exists. A new board revision used to mean
 * walking every thread and every driver and remapping pins in each, once per
 * board in the rover: the same pad spelled out in as many places as it had
 * users, with no way to tell a deliberate difference from an edit someone
 * missed. Now a revision is two edits - a line here for what a slot is wired to,
 * and a row in PROFILES for what is plugged into it on which board. Drivers ask
 * where their pads are; they no longer know.
 *
 * What is NOT a choice is everything a pad then IMPLIES: its alternate-function
 * number, which timer channel it is, whether it is the complementary output.
 * Those are fixed by the silicon - PC8 reaching I2C3 IS AF8, PB15 IS TIM15_CH2 -
 * so editing them configures nothing, it only makes a driver wrong. They are
 * stated once each, next to whatever needs them: PWM_PIN_CONFIG further down this file
 * for the PWM slots, and the probe's bus and AF in pHMeterThread.h. Never here,
 * and never twice.
 */

/** A connector's two pads. Named clk/data because that holds for both
 *  occupants: an HX711 drives SCK and reads DOUT, I2C3 drives SCL and SDA. */
struct ConnPads {
    PinId clk;
    PinId data;
};

inline constexpr ConnPads CONN_PADS[CONNECTOR_COUNT] = {
    /* ConnI2C  */ { .clk = pin::PC8, .data = pin::PC9 },
    /* ConnUART */ { .clk = pin::PA3, .data = pin::PA2 },
};

/** The single pad each PWM slot drives. */
inline constexpr PinId PWM_PADS[PWM_COUNT] = {
    /* Pwm0 */ pin::PB15,
    /* Pwm1 */ pin::PB14,
    /* Pwm2 */ pin::PB13,
    /* Pwm3 */ pin::PB11,
};

inline constexpr const ConnPads& pinOf(ConnType slot) {
    return CONN_PADS[idOf(slot) - CONNECTOR_FIRST];
}
inline constexpr PinId pwmPinOf(ConnType slot) {
    return PWM_PADS[idOf(slot) - PWM_FIRST];
}

/* ---- HAL resolvers -------------------------------------------------------- */

/** GPIO block for a port. A port handle is a pointer, not an index, so there is
 *  nothing to switch on. */
inline GPIO_TypeDef* portOf(PortId port) {
    switch (port) {
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
inline void enableGpioClock(PortId port) {
    switch (port) {
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
inline TIM_HandleTypeDef* timerOf(TimerId timer) {
    switch (timer) {
        case TimerId::Tim1:  return &htim1;
        case TimerId::Tim2:  return &htim2;
        case TimerId::Tim15: return &htim15;
        default:             return nullptr;
    }
}

/* ---- pad access, so drivers do not each rewrite it ------------------------ */

inline void writePin(PinId pin, GPIO_PinState state) {
    HAL_GPIO_WritePin(portOf(pin.port), maskOf(pin), state);
}

inline GPIO_PinState readPin(PinId pin) {
    return HAL_GPIO_ReadPin(portOf(pin.port), maskOf(pin));
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
inline void initI2c(I2C_HandleTypeDef& handle, BusId bus, uint32_t timing) {
    enableBusClock(bus);
    handle.Instance              = busOf(bus);
    handle.Init.Timing           = timing;
    handle.Init.OwnAddress1      = 0;
    handle.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    handle.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    handle.Init.OwnAddress2      = 0;
    handle.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    handle.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    handle.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    (void)HAL_I2C_Init(&handle);
}

/** Release a pad back to its reset state. */
inline void deinitPin(PinId pin) {
    HAL_GPIO_DeInit(portOf(pin.port), maskOf(pin));
}

/** Claim a pad. `af` is ignored unless the mode is an alternate function. */
inline void configPin(PinId pin, uint32_t mode, uint32_t pull,
                      uint32_t speed = GPIO_SPEED_FREQ_LOW, uint8_t af = 0) {
    enableGpioClock(pin.port);
    GPIO_InitTypeDef gpio = {};
    gpio.Pin       = maskOf(pin);
    gpio.Mode      = mode;
    gpio.Pull      = pull;
    gpio.Speed     = speed;
    gpio.Alternate = af;
    HAL_GPIO_Init(portOf(pin.port), &gpio);
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
/** A timer's capture/compare channel. Values ARE the hardware's 1..4, so nothing
 *  needs mapping - the type exists to stop the number being anything else. */
enum class TimCh : uint8_t { Ch1 = 1, Ch2 = 2, Ch3 = 3, Ch4 = 4 };

/**
 * A pad's alternate-function number.
 */
enum class AltFunc : uint8_t {
    Af0 = 0, Af1, Af2,  Af3,  Af4,  Af5,  Af6,  Af7,
    Af8,     Af9, Af10, Af11, Af12, Af13, Af14, Af15
};

struct PwmPinConfig {
    PinId   pin;
    TimerId timer;
    TimCh   channel;
    AltFunc af;
    bool    complementary;  ///< drive CHxN rather than CHx
    bool    ccDma;          ///< the DMAMUX has a per-channel CC request here
};

/*
 * `ccDma` is the DMAMUX request table, not a preference: a channel can only
 * stream a WS2812 frame if the DMAMUX offers a per-channel CC request for it. The
 * general-purpose timers offer all four (DMA_REQUEST_TIM1_CH1..CH4,
 * TIM2_CH1..CH4), but TIM15 is a limited timer - the table stops at
 * DMA_REQUEST_TIM15_CH1, then UP/TRIG/COM. There is no TIM15_CH2 request, which
 * is why CubeMX does not offer one and why Pwm0 cannot carry the strip.
 *
 * (TIM15_UP could drive a frame from the update event instead, but that is a
 * different transfer model than show() implements, so it is not claimed here.)
 */
inline constexpr PwmPinConfig PWM_PIN_CONFIG[PWM_COUNT] = {
    /* Pwm0 - TIM15_CH2 has no DMAMUX CC request, so this slot can never carry the strip */
    { .pin = pwmPinOf(ConnType::Pwm0), .timer = TimerId::Tim15, .channel = TimCh::Ch2, .af = AltFunc::Af1, .complementary = false, .ccDma = false },
    /* Pwm1 */
    { .pin = pwmPinOf(ConnType::Pwm1), .timer = TimerId::Tim15, .channel = TimCh::Ch1, .af = AltFunc::Af1, .complementary = false, .ccDma = true  },
    /* Pwm2 - drives CH1N, the complementary output */
    { .pin = pwmPinOf(ConnType::Pwm2), .timer = TimerId::Tim1,  .channel = TimCh::Ch1, .af = AltFunc::Af6, .complementary = true,  .ccDma = true  },
    /* Pwm3 */
    { .pin = pwmPinOf(ConnType::Pwm3), .timer = TimerId::Tim2,  .channel = TimCh::Ch4, .af = AltFunc::Af1, .complementary = false, .ccDma = true  },
};

inline constexpr const PwmPinConfig& pwmConfigOf(ConnType c) { return PWM_PIN_CONFIG[idOf(c) - PWM_FIRST]; }

/** The HAL's channel constant. TIM_CHANNEL_x is not an arithmetic sequence, so
 *  it is a lookup rather than a cast. */
inline constexpr uint32_t halChannel(TimCh channel) {
    return channel == TimCh::Ch1 ? TIM_CHANNEL_1
         : channel == TimCh::Ch2 ? TIM_CHANNEL_2
         : channel == TimCh::Ch3 ? TIM_CHANNEL_3
         :                         TIM_CHANNEL_4;
}

/* What a caller driving a PWM slot actually needs, without unpacking the config. */
inline TIM_HandleTypeDef* pwmTimerOf(ConnType c)   { return timerOf(pwmConfigOf(c).timer); }
inline uint32_t           pwmChannelOf(ConnType c) { return halChannel(pwmConfigOf(c).channel); }

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////--CHECKS--DOWN--///////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/* ---- the pads are sane ---------------------------------------------------- */

/** A pad is one of 16 bits on a real port. Catches a typo'd bit outright. */
constexpr bool pinsInRange() {
    for (const ConnPads& conn : CONN_PADS)
        if (conn.clk.bit > 15 || conn.data.bit > 15) return false;
    for (const PinId& pad : PWM_PADS)
        if (pad.bit > 15) return false;
    return true;
}
static_assert(pinsInRange(), "a slot names a pad outside bits 0..15");

/**
 * No pad is wired to two slots.
 *
 * This is the check the old HAL-pointer table could not express at all: GPIOC is
 * an integer-to-pointer cast, so nothing about a pad was a constant expression.
 * Encoding pads as PortId + bit is what buys this - and it is the mistake most
 * worth catching, because two slots sharing a pad produces two drivers quietly
 * reconfiguring the same pin, last one wins, no symptom until the hardware
 * behaves strangely.
 */
constexpr bool noPadUsedTwice() {
    PinId seen[CONNECTOR_COUNT * 2 + PWM_COUNT] = {};
    uint8_t count = 0;

    for (const ConnPads& conn : CONN_PADS) {
        const PinId pads[2] = { conn.clk, conn.data };
        for (const PinId& pad : pads) {
            for (uint8_t i = 0; i < count; ++i)
                if (samePin(seen[i], pad)) return false;
            seen[count++] = pad;
        }
    }
    for (const PinId& pad : PWM_PADS) {
        for (uint8_t i = 0; i < count; ++i)
            if (samePin(seen[i], pad)) return false;
        seen[count++] = pad;
    }
    return true;
}
static_assert(noPadUsedTwice(), "two slots are wired to the same pad");

/* The strip streams over a per-channel DMA request, so its slot must have one. */
static_assert(pwmConfigOf(LED_STRIP_SLOT).ccDma,
              "LED_STRIP_SLOT has no per-channel DMA request (TIM15_CH2 has none) - "
              "the strip cannot stream from that slot");

/**
 * The strip must own its timer outright, among the PWM slots.
 *
 * Stronger than noTimerBaseConflict() in PWMDriver.h, and for a different
 * reason. That one is per board: it fires only once some row actually puts a
 * servo beside the strip. This one is structural - a strip on a timer that a
 * second slot also uses makes that slot unusable for a servo on EVERY board,
 * because the two want time bases four orders of magnitude apart. Better to
 * refuse the placement than to discover a slot is dead only when a board needs
 * it.
 *
 * With today's wiring the admissible slots are Pwm2 (TIM1) and Pwm3 (TIM2): Pwm0
 * and Pwm1 share TIM15, so either would cost the other. That pair is DERIVED
 * here rather than written down - move a slot to another timer in PWM_PIN_CONFIG and
 * the admissible set follows on its own.
 */
constexpr bool stripOwnsItsTimer() {
    const uint8_t self = idOf(LED_STRIP_SLOT) - PWM_FIRST;
    for (uint8_t i = 0; i < PWM_COUNT; ++i)
        if (i != self && PWM_PIN_CONFIG[i].timer == PWM_PIN_CONFIG[self].timer) return false;
    return true;
}
static_assert(stripOwnsItsTimer(),
              "LED_STRIP_SLOT shares a timer with another PWM slot (Pwm0/Pwm1 are both "
              "TIM15) - that slot could never hold a servo; put the strip on Pwm2 or Pwm3");

/**
 * How many slots the derived rules above would accept.
 *
 * The two constraints that are real - a per-channel DMA request (ccDma) and sole
 * ownership of a timer - do NOT single out one slot: with today's wiring they admit
 * Pwm2 (TIM1) and Pwm3 (TIM2) equally. Counting them is what makes the next
 * assertion honest, and what makes it fail loudly if the wiring ever changes: if a
 * rewiring drops the count to 1, the pin below is redundant and should go; if it
 * rises, the choice widened and someone should say which slot wins and why.
 */
constexpr uint8_t admissibleStripSlots() {
    uint8_t n = 0;
    for (uint8_t i = 0; i < PWM_COUNT; ++i) {
        if (!PWM_PIN_CONFIG[i].ccDma) continue;
        bool alone = true;
        for (uint8_t j = 0; j < PWM_COUNT; ++j)
            if (j != i && PWM_PIN_CONFIG[j].timer == PWM_PIN_CONFIG[i].timer) alone = false;
        if (alone) ++n;
    }
    return n;
}
static_assert(admissibleStripSlots() == 2,
              "the admissible strip slots are no longer {Pwm2, Pwm3} - revisit the pin in BoardUtils.h");
