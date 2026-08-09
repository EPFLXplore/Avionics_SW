/*
 * BoardProfile.h  --  the one file that says what lives where.
 *
 * One firmware image runs on every master; the 2-bit strap read by
 * Board_MasterId() selects a row below. Each entry is a LOCAL slot on this
 * board - a load-cell connector, or a PWM channel - holding the GLOBAL device
 * id plugged into it, or NO_DEVICE when nothing is.
 *
 * One entry per slot, so a slot cannot hold two things: occupancy is enforced by
 * the array, not by a check somebody has to remember to write. The array NAME
 * says what kind of device belongs there, which is why no device-kind tag is
 * needed - mass[] carries MassIds, servo[] carries ServoIds.
 *
 * Servos are indexed by PWM CHANNEL, not by connector. Which connector a channel
 * comes off is a wiring fact and lives in ServoConfigs.h with the pin and timer,
 * so a second servo connector is "bump SERVO_CHANNEL_COUNT, add configs, add
 * ids" with no structural change here.
 *
 * Threads read this once, in init(), to bind their local objects to a global id.
 * Nothing consults it afterwards, and nothing above the threads reads it at all:
 * System asks a thread whether it has devices, never the table. When hardware
 * detection arrives it replaces (or gates) that one bind and nothing else moves.
 *
 * Edit a row when hardware moves, and reflash.
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "device_ids.h"
#include "Bridge.h"   // Board_MasterId()

/* ---- local slot vocabulary ---------------------------------------------- */

/**
 * WHICH CONNECTOR, not which device. The HX711 driver bit-bangs, so any GPIO
 * pair works and a cell can sit on any connector; these name the two wired
 * today. Used to index mass[] below.
 */
enum class MassConnectors : uint8_t {
    I2C  = 0,
    UART = 1,
};
constexpr uint8_t MASS_CONNECTOR_COUNT = 2;

/**
 * WHICH PWM OUTPUT, not which device. CHn drives SERVO_n_CFG - see
 * ServoConfigs.h for its pin and timer. Used to index servo[] below.
 */
enum class ServoChannel : uint8_t {
    CH0 = 0,
    CH1 = 1,
    CH2 = 2,
    CH3 = 3,
};
constexpr uint8_t SERVO_CHANNEL_COUNT = 4;

static_assert(id_of(MassConnectors::UART) + 1 == MASS_CONNECTOR_COUNT,
              "MassConnectors and MASS_CONNECTOR_COUNT disagree");
static_assert(id_of(ServoChannel::CH3) + 1 == SERVO_CHANNEL_COUNT,
              "ServoChannel and SERVO_CHANNEL_COUNT disagree");

/* ---- the table ----------------------------------------------------------- */

struct BoardProfile {
    uint8_t mass[MASS_CONNECTOR_COUNT];   // MassId  per connector, or NO_DEVICE
    uint8_t servo[SERVO_CHANNEL_COUNT];   // ServoId per PWM channel, or NO_DEVICE

    /**
     * A WS2812 strip is fitted, and it owns TIM15 (PSC/ARR/DMA).
     *
     * A bool rather than an id because a strip is not addressable: LEDRequest
     * carries no id field, so there is nothing to name it with. It still
     * belongs here, because "this board runs the strip" is the same fact that
     * must keep TIM15 clear of servos - see noTim15Conflict() in ServoConfigs.h,
     * which rejects a row claiming both at compile time.
     */
    bool led_strip;
};

/** Comms only: nothing plugged into any slot. */
constexpr BoardProfile EMPTY_BOARD = {
    { NO_DEVICE, NO_DEVICE },
    { NO_DEVICE, NO_DEVICE, NO_DEVICE, NO_DEVICE },
    false
};

/* Plain constexpr, not `inline constexpr`: inline variables are C++17 and this
 * project builds at -std=gnu++14, where GCC accepts them only as an extension
 * (six -Wc++17-extensions warnings). Internal linkage means one copy per
 * translation unit instead of one shared - a handful of bytes of rodata for a
 * table this size, and the same trade EMPTY_BOARD above already makes. */
constexpr BoardProfile PROFILES[4] = {
    /* 0 */ { { id_of(MassId::SandRocks),          // I2C connector
                id_of(MassId::Drill) },            // UART connector
              { id_of(ServoId::FrontCam),          // CH0
                id_of(ServoId::Drill),             // CH1
                id_of(ServoId::LeftServiceModule), // CH2
                id_of(ServoId::RightServiceModule) },
              false },
    /* 1 */ EMPTY_BOARD,
    /* 2 */ EMPTY_BOARD,
    /* 3 */ { { NO_DEVICE, NO_DEVICE },            // LED master: the strip owns
              { NO_DEVICE, NO_DEVICE,              // TIM15, so CH0/CH1 are
                NO_DEVICE, NO_DEVICE },            // unavailable; CH2/CH3 are
              true },                              // free but unused today
};

/** This board's row. Board_MasterId() is strapped on two pins, so it is 0..3. */
inline const BoardProfile& profile() { return PROFILES[Board_MasterId()]; }

/* ---- helpers, one template each ------------------------------------------ */

/**
 * Does this row hold anything in the given slot array?
 *
 * Answered straight off the profile, NOT off a bound device array, so a thread
 * can be asked before it has bound anything. That is what lets binding stay in
 * init() (task context, after the scheduler) while System still decides at
 * start time whether to start the thread at all - the same order HEAD had, when
 * the decision was a live `switch (Board_MasterId())` in System::init.
 */
template <std::size_t N>
inline bool anyDevice(const uint8_t (&ids)[N]) {
    for (uint8_t id : ids)
        if (id != NO_DEVICE) return true;
    return false;
}

/**
 * Bind every device in a local array to the global id in the matching profile
 * slot. The shared N is the point: a device array and its id array must be the
 * same length or this does not compile, which is a stronger guarantee than the
 * assert it replaces.
 */
template <class T, std::size_t N>
inline void bindDevices(T (&devices)[N], const uint8_t (&ids)[N]) {
    for (std::size_t i = 0; i < N; ++i) devices[i].global_id = ids[i];
}

/**
 * The device answering to a global id, or nullptr when this board has none -
 * which is also how a broadcast command for another board gets dropped.
 * NO_DEVICE is rejected up front: unbound slots all hold it, so it would
 * otherwise match the first empty one.
 */
template <class T, std::size_t N>
inline T* deviceFor(T (&devices)[N], uint8_t global_id) {
    if (global_id == NO_DEVICE) return nullptr;
    for (T& d : devices)
        if (d.global_id == global_id) return &d;
    return nullptr;
}

/**
 * No connector on ONE board is claimed by two device kinds at once.
 *
 * Scope matters here, and it is the opposite of noDuplicateIds() below. A device id
 * is fleet-wide unique - there is one drill scale in the rover. A connector is
 * per board: board 0's I2C connector and board 2's I2C connector are different
 * physical parts, so both may be occupied. Only one board claiming its own
 * connector twice is a conflict, so this compares two arrays FROM THE SAME ROW.
 *
 * Occupancy WITHIN one array is already structural - a slot is a single uint8_t
 * and cannot hold two devices. This covers the other axis: two arrays indexed by
 * the same connector space, e.g. mass[] and a future i2c-sensor array both
 * wanting the I2C connector, whose pins can be bit-banged for an HX711 or handed
 * to the I2C peripheral, never both.
 *
 * The shared N is doing real work: two arrays can only be compared if they are
 * the same length, i.e. indexed by the same thing. mass[] (2 connectors) against
 * servo[] (4 PWM channels) will not compile, which is correct - those spaces do
 * not overlap and comparing them would be meaningless.
 *
 * No call site yet: mass[] is currently the only array over connectors. When a
 * second kind lands on that space, check every row:
 *
 *     constexpr bool i2cExclusive() {
 *         for (const BoardProfile& p : PROFILES)
 *             if (!noContention(p.mass, p.ph)) return false;
 *         return true;
 *     }
 *     static_assert(i2cExclusive(), "a board claims one connector twice");
 */
template <std::size_t N>
constexpr bool noContention(const uint8_t (&a)[N], const uint8_t (&b)[N]) {
    for (std::size_t i = 0; i < N; ++i)
        if (a[i] != NO_DEVICE && b[i] != NO_DEVICE) return false;
    return true;
}

/**
 * A global id may appear at most ONCE across the whole fleet. Two boards
 * claiming one device is what produces two scales publishing under a single id,
 * or a command moving the wrong actuator - and both look like flaky hardware
 * from the outside, so they are checked here rather than hoped for.
 *
 * Takes a pointer-to-member-array so mass[] and servo[] share one implementation
 * while staying separate id spaces (MassId::Drill and ServoId::Drill are both 1).
 */
template <std::size_t N>
constexpr bool noDuplicateIds(uint8_t (BoardProfile::*slots)[N]) {
    uint8_t seen[N * 4] = {};
    uint8_t n = 0;
    for (const BoardProfile& p : PROFILES) {
        for (uint8_t id : p.*slots) {
            if (id == NO_DEVICE) continue;
            for (uint8_t i = 0; i < n; ++i)
                if (seen[i] == id) return false;
            seen[n++] = id;
        }
    }
    return true;
}

static_assert(noDuplicateIds(&BoardProfile::mass),  "two boards claim the same mass device");
static_assert(noDuplicateIds(&BoardProfile::servo), "two boards claim the same servo device");
