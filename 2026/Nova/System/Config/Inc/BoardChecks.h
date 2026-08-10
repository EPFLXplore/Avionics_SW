/*
 * BoardChecks.h  --  every rule the board table must satisfy, proved at compile
 *                    time.
 *
 * Split out of BoardProfile.h so that file stays a readable description of the
 * hardware. This one is the schema: the Zephyr split, where the devicetree says
 * what exists and the bindings say what is legal.
 *
 * NOT a standalone header. BoardProfile.h includes it as its last line, because
 * every check below reads the table defined above it - and because a header of
 * static_asserts nobody includes is a set of guarantees that silently do not
 * exist. Do not include it directly; include BoardProfile.h.
 *
 * Everything here is constexpr and evaluated during compilation: none of it
 * costs a byte of flash or a cycle at runtime.
 *
 * WHAT BELONGS HERE, and what does not: a check must be able to see its data, so
 * placement follows the data rather than being a filing choice. Checks over the
 * BOARD TABLE - pads and occupancy - live here. Checks over a driver's own
 * invariants live with that driver, because nothing else can see them: the angle
 * map and the timer-base rule are in PWMDriver.h for exactly that reason, and
 * pulling them here would make this file depend on a driver, backwards from
 * every other arrow in the system. When a check spans both, it goes with the
 * more specific data. The side benefit is that a failure lands near the file you
 * were editing when you broke it.
 */

#pragma once

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////--CHECKS--DOWN--///////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/* ---- slot space layout ---------------------------------------------------- */

/* Board_MasterId() reads two strap pins, so it returns 0..3, and profile() uses
 * that as a bare array index with no bounds check. One row per strap value or a
 * board that physically exists reads past the end of the table. */
constexpr uint8_t BOARD_STRAP_VALUES = 4;   // 2 strap bits on PB4/PB5
static_assert(sizeof(PROFILES) / sizeof(PROFILES[0]) == BOARD_STRAP_VALUES,
              "PROFILES must have exactly one row per 2-bit board strap value");

/* bindSlots() binds a device array to a contiguous run of slots, so the groups
 * have to actually be contiguous and in this order. */
static_assert(idOf(ConnType::Pwm3) + 1 == SLOT_COUNT, "ConnType and SLOT_COUNT disagree");
static_assert(CONNECTOR_FIRST + CONNECTOR_COUNT == PWM_FIRST,
              "connector slots must be contiguous and come before the PWM slots");
static_assert(PWM_FIRST + PWM_COUNT == SLOT_COUNT,
              "PWM slots must be contiguous and end the slot space");

/* ---- the pads are sane ---------------------------------------------------- */

/** A pad is one of 16 bits on a real port. Catches a typo'd bit outright. */
constexpr bool pinsInRange() {
    for (const ConnPads& c : CONN_PADS)
        if (c.clk.bit > 15 || c.data.bit > 15) return false;
    for (const PinId& p : PWM_PADS)
        if (p.bit > 15) return false;
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
    uint8_t n = 0;

    for (const ConnPads& c : CONN_PADS) {
        const PinId pads[2] = { c.clk, c.data };
        for (const PinId& p : pads) {
            for (uint8_t i = 0; i < n; ++i)
                if (samePin(seen[i], p)) return false;
            seen[n++] = p;
        }
    }
    for (const PinId& w : PWM_PADS) {
        for (uint8_t i = 0; i < n; ++i)
            if (samePin(seen[i], w)) return false;
        seen[n++] = w;
    }
    return true;
}
static_assert(noPadUsedTwice(), "two slots are wired to the same pad");

/* ---- placement: which kind may sit in which slot -------------------------- */

/**
 * May this kind sit in this slot? The single statement of driver placement.
 *
 * Mostly not policy. An ADS1114 needs a real I2C peripheral, and only ConnI2C's
 * pads reach one; HX711 bit-bangs, so a load cell is happy on either connector.
 * A row that breaks either describes hardware that cannot exist.
 *
 * The WS2812 line is the exception, and is a CONFIGURATION fact rather than a
 * pad fact: the strip needs a channel whose DMA request the .ioc maps. That is
 * why it reads LED_STRIP_SLOT rather than a literal - moving the strip is an
 * edit to that one constant (plus CubeMX), not to this function.
 */
constexpr bool deviceFitsSlot(DeviceType d, uint8_t s) {
    const bool isConnector = (s < CONNECTOR_FIRST + CONNECTOR_COUNT);
    const bool isPwm       = (s >= PWM_FIRST);

    return d == DeviceType::None     ? true
         : d == DeviceType::LoadCell ? isConnector
         : d == DeviceType::PhMeter  ? (s == idOf(ConnType::ConnI2C))
         : d == DeviceType::Servo    ? isPwm
         : d == DeviceType::LedStrip ? (s == idOf(LED_STRIP_SLOT))
         : false;
}

constexpr bool placementsValid() {
    for (const BoardProfile& p : PROFILES)
        for (std::size_t i = 0; i < SLOT_COUNT; ++i)
            if (!deviceFitsSlot(p.slots[i].device, static_cast<uint8_t>(i))) return false;
    return true;
}
static_assert(placementsValid(),
              "a board puts a device in a slot whose pads cannot drive it");

/* ---- ids ------------------------------------------------------------------ */

/** Does this kind carry an id on the wire? LoadCell publishes under a MassIdType
 *  and Servo is commanded by a ServoIdType; PhMeter and LedStrip are singletons,
 *  so their packets have no id field at all. Only idsMatchDevice() needs this,
 *  which is why it lives here rather than beside the table. */
constexpr bool carriesWireId(DeviceType d) {
    return d == DeviceType::LoadCell || d == DeviceType::Servo;
}

/**
 * A slot's id agrees with its kind: addressable kinds carry one, singletons do
 * not. Catches both halves - a load cell left at NO_DEVICE (it would publish
 * under the empty sentinel) and a strip given an id (nothing on the wire would
 * ever carry it, so it could only mislead).
 */
constexpr bool idsMatchDevice() {
    for (const BoardProfile& p : PROFILES)
        for (const Slot& s : p.slots)
            if (carriesWireId(s.device) != (s.id != NO_DEVICE)) return false;
    return true;
}
static_assert(idsMatchDevice(),
              "a slot's id does not match its device kind (wire-addressed kinds need one, singletons have none)");

/**
 * A global id appears at most ONCE across the fleet, per kind. Two boards
 * claiming one device is what produces two scales publishing under a single id,
 * or a command moving the wrong actuator - and both look like flaky hardware
 * from the outside, so they are checked here rather than hoped for.
 */
constexpr bool noDuplicateIds(DeviceType kind) {
    uint8_t seen[SLOT_COUNT * 4] = {};
    uint8_t n = 0;
    for (const BoardProfile& p : PROFILES)
        for (const Slot& s : p.slots) {
            if (s.device != kind || s.id == NO_DEVICE) continue;
            for (uint8_t i = 0; i < n; ++i)
                if (seen[i] == s.id) return false;
            seen[n++] = s.id;
        }
    return true;
}
static_assert(noDuplicateIds(DeviceType::LoadCell), "two boards claim the same load cell");
static_assert(noDuplicateIds(DeviceType::Servo),    "two boards claim the same servo");

/* An id says nothing about where a device sits, by design - see device_ids.h.
 * Placement is checked here (deviceFitsSlot, the timer rule in PWMDriver.h) and
 * identity is checked here (uniqueness, kind agreement); the two never have to
 * agree with each other. */

/* The one rule that spans slots - a strip and a servo cannot share a timer's
 * time base - needs to know which slots share a timer, which is a PWM wiring
 * fact. It lives with that wiring, in PWMDriver.h. */
