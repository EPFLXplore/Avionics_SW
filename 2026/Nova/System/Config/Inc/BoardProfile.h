/*
 * BoardProfile.h  --  the one file that says what the hardware is.
 *
 * Two things live here, and they are the same concern: what each SLOT physically
 * IS (its pads and the peripheral behind them), and WHAT IS PLUGGED IN to each
 * slot on each board. One firmware image runs on every master; the 2-bit strap
 * read by Board_MasterId() selects a row.
 *
 * A board is a fixed set of slots - physical attachment points. Every slot holds
 * at most one DeviceType, so occupancy is structural: claiming a slot twice is
 * unwriteable rather than caught. `id` names the device where its kind is
 * addressable, and is NO_DEVICE where the kind carries none on the wire.
 *
 * NO HAL HERE, deliberately. Pads are PortId + bit, peripherals are BusId and
 * TimerId - plain enums, not GPIOC and htim15. Those are integer-to-pointer
 * casts and therefore not constant expressions, and holding one would make the
 * whole table unreadable at compile time. Keeping it pure data is exactly what
 * lets BoardChecks.h prove things about it. The HAL pointers get resolved in
 * Pins.h, at the one moment a driver actually needs them.
 *
 * The .ioc is NOT the source of truth for these pads - the PCB is, and this file
 * records it. Every driver configures its own pads (HX711::begin,
 * ADS1114::begin, PWMDriver's constructor), so CubeMX's GPIO setup for them is
 * overwritten moments later; it exists only to keep the pads quiet until then.
 *
 * Threads read this once, in init(), to bind their local objects. Nothing
 * consults it afterwards, and nothing above the threads reads it at all: System
 * asks a thread whether it has devices, never the table.
 *
 * Edit a row when hardware moves, and reflash.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "device_ids.h"
#include "Bridge.h"   // Board_MasterId()

/* ---- pad vocabulary ------------------------------------------------------- */

enum class PortId : uint8_t { A = 0, B, C, D, E, F, G };

/** A pad, as pure data. `bit` is 0..15; maskOf() turns it into a HAL pin mask. */
struct PinId {
    PortId  port;
    uint8_t bit;
};

constexpr uint16_t maskOf(PinId p)          { return static_cast<uint16_t>(1u << p.bit); }
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

/* ---- the slot space ------------------------------------------------------- */

/**
 * Every attachment point on a master.
 *
 * Ordering matters: the connector slots come first and the PWM slots after, each
 * group contiguous, so a thread's device array can be bound from a slot RANGE
 * (see bindSlots, and the static_asserts in BoardChecks.h).
 */
enum class ConnType : uint8_t {
    ConnI2C = 0,  ///< a bit-banged pad pair, or I2C3
    ConnUART,     ///< a bit-banged pad pair; no I2C peripheral reaches these pads
    Pwm0,
    Pwm1,
    Pwm2,
    Pwm3,
};
constexpr uint8_t SLOT_COUNT = 6;

/**
 * The PWM slot the WS2812 strip is driven from. ONE constant, three consumers:
 * deviceFitsSlot() (which slot a strip may occupy), LedsThread::init() (which
 * timer and channel the driver is handed), and the reader of a board row.
 *
 * A CONFIGURATION fact, not a pad fact. Any PWM slot can drive the strip; what
 * makes it this one is that CubeMX maps that channel's DMA request. Moving it is
 * therefore TWO edits that must agree:
 *
 *   1. this line, and
 *   2. the DMA request in Nova.ioc - today `Dma.Request0 = TIM2_CH4` (DMA1_Channel3,
 *      hdma[TIM_DMA_ID_CC4] of htim2) and `Dma.Request1 = TIM1_CH1` (DMA1_Channel4,
 *      hdma[TIM_DMA_ID_CC1] of htim1). Both are wired in Core/Src/tim.c, so Pwm2
 *      and Pwm3 are the two slots that can actually stream a frame today.
 *
 * Nothing in C++ can see the .ioc, so the second half is on you; the driver
 * itself no longer cares, since it derives its DMA request and CC flag from
 * whatever channel it is handed.
 *
 * Choosing a slot is not free: it must be on a timer no servo shares, because a
 * timer has one time base and the strip's is 800 kHz. See noTimerBaseConflict()
 * in PWMDriver.h, which proves that for every board row.
 *
 * Pinned to Pwm2 - see the LED_STRIP_SLOT static_assert in Pins.h for why this is
 * a decision rather than something derived, and what has to change to move it.
 */
inline constexpr ConnType LED_STRIP_SLOT = ConnType::Pwm2;

constexpr uint8_t CONNECTOR_FIRST = idOf(ConnType::ConnI2C);
constexpr uint8_t CONNECTOR_COUNT = 2;
constexpr uint8_t PWM_FIRST       = idOf(ConnType::Pwm0);
constexpr uint8_t PWM_COUNT       = 4;

/* ---- which pads each slot uses -------------------------------------------- */
/*
 * A CHOICE: this is the PCB, and changing a line here means the board changed.
 *
 * What is deliberately NOT here is everything the pad then implies - the
 * alternate-function number, which timer channel a pad is, whether it is the
 * complementary output. Those are fixed by the silicon: PC8 reaching I2C3 IS
 * AF8, PB15 IS TIM15_CH2. Editing them would not configure anything, it would
 * only make a driver wrong, so they live with the driver that needs them
 * (PWMConfig in PWMDriver.h, the bus in pHMeterThread.h).
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

inline constexpr const ConnPads& pinOf(ConnType c) {
    return CONN_PADS[idOf(c) - CONNECTOR_FIRST];
}
inline constexpr PinId pwmPinOf(ConnType c) {
    return PWM_PADS[idOf(c) - PWM_FIRST];
}

/* ---- what can occupy a slot ----------------------------------------------- */

/**
 * The kind of device in a slot.
 *
 * Where each kind may sit is a property of its DRIVER, not of any board, so it
 * is stated once in deviceFitsSlot() (BoardChecks.h) rather than trusted per
 * row: HX711 bit-bangs and goes on either connector; ADS1114 needs a real I2C
 * peripheral, so it only fits a connector whose `bus` is not None; a servo works
 * on any PWM output; the WS2812 strip needs the channel wired to its DMA
 * request, which is LED_STRIP_SLOT alone.
 */
enum class DeviceType : uint8_t {
    None = 0,   ///< nothing plugged in
    LoadCell,   ///< HX711, bit-banged   -> MassThread     | carries a MassIdType
    PhMeter,    ///< ADS1114 over I2c3   -> pHMeterThread  | no id (one in the rover)
    Servo,      ///< 50 Hz PWM           -> ServoThread    | carries a ServoIdType
    LedStrip,   ///< WS2812 over PWM+DMA -> LedsThread     | no id (one in the rover)
};






/** One slot's occupancy: what is in it, and which device that is where the kind
 *  is addressable. Indexed by ConnType, stored as slots[] on the profile. */
struct Slot {
    DeviceType device;
    uint8_t    id;
};

/** Nothing plugged in. */
inline constexpr Slot FREE = { .device = DeviceType::None, .id = NO_DEVICE };

/* ---- the table ------------------------------------------------------------ */

struct BoardProfile {
    Slot slots[SLOT_COUNT];
};

/** Comms only: nothing plugged into any slot. */
inline constexpr BoardProfile EMPTY_BOARD = { .slots = { FREE, FREE, FREE, FREE, FREE, FREE } };

/*
 * The rows themselves.
 *
 * Wrapped in a namespace purely so the short type aliases below stay local: the
 * long names read well everywhere else, but here the kind repeats on every line
 * and buries the one thing each line is actually saying. `using enum` would be
 * the real answer - it is C++20, and this builds at -std=gnu++17.
 */
namespace board_table {

using Dev  = DeviceType;
using Mass = MassIdType;
using Srv  = ServoIdType;

inline constexpr BoardProfile PROFILES[4] = {

    /* 0 - drill master: the drill load cell and the drill servo, nothing else.
     *     The sand/rocks load cell moved to board 3's ConnUART. */
    { .slots = {
        { .device = Dev::LoadCell, .id = idOf(Mass::Drill) },  // ConnI2C
        FREE,                                                  // ConnUART
        { .device = Dev::Servo,    .id = idOf(Srv::Drill)  },  // Pwm0
        FREE,                                                  // Pwm1
        FREE,                                                  // Pwm2
        FREE,                                                  // Pwm3
    } },

    /* 1 */ EMPTY_BOARD,
    /* 2 */ EMPTY_BOARD,

    /* 3 - LED master, now carrying the servos as well. Every slot is occupied:
     *     a pH probe on the I2C connector, the sand/rocks load cell on the UART
     *     connector, three servos, and the strip.
     *
     *     THE STRIP TAKES Pwm2 (LED_STRIP_SLOT), and that placement is the whole
     *     reason this row builds. Pwm2 is TIM1, which no servo here uses, so the
     *     strip owns its time base outright. The two servos on Pwm0/Pwm1 share
     *     TIM15 with each other, which is fine - they want the same 50 Hz frame
     *     and differ only in CCR - and FrontCam has TIM2 to itself on Pwm3.
     *
     *     Any other arrangement fails: with four slots occupied and Pwm0/Pwm1
     *     both on TIM15, putting the strip on either of those forces a servo
     *     onto TIM15 too, and one timer cannot be both 800 kHz and 50 Hz.
     *
     *     Pwm2 drives CH1N (complementary) - PwmMux::complementary carries that
     *     to the driver, which picks HAL_TIMEx_PWMN_Start_DMA accordingly. */
    { .slots = {
        { .device = Dev::PhMeter,  .id = NO_DEVICE                     },  // ConnI2C
        { .device = Dev::LoadCell, .id = idOf(Mass::SandRocks)         },  // ConnUART
        { .device = Dev::Servo,    .id = idOf(Srv::RightServiceModule) },  // Pwm0 - TIM15
        { .device = Dev::Servo,    .id = idOf(Srv::LeftServiceModule)  },  // Pwm1 - TIM15
        { .device = Dev::LedStrip, .id = NO_DEVICE                     },  // Pwm2 - TIM1 CH1N, strip alone
        { .device = Dev::Servo,    .id = idOf(Srv::FrontCam)           },  // Pwm3 - TIM2
    } },
};

} // namespace board_table

using board_table::PROFILES;

/** This board's row. Board_MasterId() is strapped on two pins, so it is 0..3. */
inline const BoardProfile& profile() { return PROFILES[Board_MasterId()]; }

/* ---- helpers -------------------------------------------------------------- */

/**
 * Does this board carry any device of this kind?
 *
 * Answered straight off the profile, NOT off a bound device array, so a thread
 * can be asked before it has bound anything. That is what lets binding stay in
 * init() (task context, after the scheduler) while System still decides at start
 * time whether to start the thread at all. Every thread's hasDevices() is this
 * one call, whether it owns an array of devices or a single one.
 */
inline bool anySlot(DeviceType kind) {
    for (const Slot& s : profile().slots)
        if (s.device == kind) return true;
    return false;
}

/**
 * Bind a device array to a contiguous run of slots starting at `first`.
 *
 * devices[i] takes the id in slot first+i when that slot holds `kind`, and
 * NO_DEVICE otherwise - so an array only ever answers for slots that actually
 * carry its kind, and a slot holding something else leaves its device inert.
 */
template <class T, std::size_t N>
inline void bindSlots(T (&devices)[N], DeviceType kind, ConnType first) {
    for (std::size_t i = 0; i < N; ++i) {
        const Slot& s = profile().slots[idOf(first) + i];
        devices[i].globalId = (s.device == kind) ? s.id : NO_DEVICE;
    }
}

/**
 * The device answering to a global id, or nullptr when this board has none -
 * which is also how a broadcast command for another board gets dropped.
 * NO_DEVICE is rejected up front: unbound slots all hold it, so it would
 * otherwise match the first empty one.
 */
template <class T, std::size_t N>
inline T* deviceFor(T (&devices)[N], uint8_t globalId) {
    if (globalId == NO_DEVICE) return nullptr;
    for (T& d : devices)
        if (d.globalId == globalId) return &d;
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////--CHECKS--DOWN--///////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/*
 * Every rule this table must satisfy.
 *
 * Included LAST, and from here rather than left to callers, because a header of
 * static_asserts that nobody includes is a set of guarantees that silently do
 * not exist. It reads everything defined above, so the position is load-bearing.
 */
#include "BoardChecks.h"
