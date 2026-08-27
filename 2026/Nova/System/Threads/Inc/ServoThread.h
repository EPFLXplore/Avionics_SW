#pragma once
#include "MessageThread.h"
#include "packets.h"
#include "device_ids.h"    // ServoId: the shared, fleet-wide device ids
#include "BoardProfile.h"  // which slot carries which servo, per board
#include "PWMDriver.h"

// ConnType / PWM_COUNT live in BoardProfile.h, beside the table they index.
// Only the wire speaks ServoId; everything here speaks slots.
//
// The LED strips are not servos: they go through LedsThread over LEDRequest and
// have neither a channel nor a ServoId.

/**
 * Servos that move as ONE mechanism. A command for either member drives both.
 *
 * This is declared structure, not structure read out of the id values: the ids
 * above stay arbitrary and still say nothing about board or connector. What a
 * row here states is that two DEVICES are one moving thing, which is a fact
 * about the rover rather than about any master. Both halves must live on the
 * same board or the coupling only half-arrives - couplesShareABoard() below
 * proves that for every row.
 *
 * `mirrored` is the physical mounting: when the two face opposite ways, the
 * partner drives ANGLE_MAX_DEG - angle so both leaves travel together rather
 * than into each other. Set it from the hardware, not from taste.
 *
 * `a` is the member the rest of the system COMMANDS; `b` follows it. The names
 * carry that: ServiceModuleMechanism is the mechanism's angle, and
 * ServiceModuleMechanismMirrored is the leaf that tracks it from the other side.
 * Addressing `b` directly still works - it takes the angle literally and `a`
 * mirrors instead - but it opens the same mechanism from the opposite frame, so
 * commanding both halves in turn has the second undo the first. Command `a`.
 *
 * Only the commanded ANGLE is coupled. A home position is not: change_zero
 * addresses exactly the servo it names, so each leaf keeps its own trim - and
 * so Nexus, which replays servo_cal.yaml one id at a time, cannot have one
 * member's replay overwrite the other's. go_to_zero does reach both, each to
 * its own home.
 */
struct ServoCouple {
    uint8_t a;
    uint8_t b;
    bool    mirrored;
};

inline constexpr ServoCouple SERVO_COUPLES[] = {
    /* The service module's two leaves. Handed - they face each other across the
     * module - so the mechanism's angle goes to one and its mirror to the
     * other. */
    { idOf(ServoIdType::ServiceModuleMechanism),
      idOf(ServoIdType::ServiceModuleMechanismMirrored), true },
};

class ServoThread : public MessageThread<ServoRequest, EmptyMessage> {
public:
    ServoThread(const char* name, osPriority priority);
    virtual ~ServoThread();

    /** True when this board carries at least one servo. Read live off the
     *  profile at the moment System asks, NOT latched at construction: that
     *  keeps the start decision where HEAD had it - in System::init, at start
     *  time - instead of freezing it into a bool several statements earlier.
     *  System asks this instead of reading the profile itself, so nothing above
     *  the threads knows what a channel is. */
    bool hasDevices() const { return anySlot(DeviceType::Servo); }

    void init() override;
    void loop() override;

private:

    /**
     * Execute one request against ONE servo, mirroring the angle if asked.
     *
     * Split out of loop() because a request no longer means a single channel: a
     * member of a SERVO_COUPLES pair drives its partner too, and both go through
     * here so the coupled leaf cannot drift from the commanded one by taking a
     * different path. A servo this board does not carry is dropped exactly as
     * before - deviceFor() returns nullptr and nothing is written.
     *
     * `mirrored` flips the commanded ANGLE only. A home position is per-servo
     * (see SERVO_COUPLES above), so go_to_zero drives this servo to
     * its own zero, mirrored or not.
     */
    void apply(const ServoRequest& req, uint8_t id, bool mirrored);

    // Plain objects. Safe because ServoThread is constructed at runtime (from
    // System::init, after HAL), so PWMDriver's HAL timer setup runs post-HAL.
    //
    // A channel is constructed live only when the profile declares a device on
    // it; otherwise it is inert and touches neither pin nor timer. That is what
    // keeps TIM15 clear for the WS2812 strip on a board that declares no servos,
    // and stops boards with none from configuring timers they never use.
    /* The lamp switch's pad, resolved once in init(). LampPower sits in a PWM
     * slot and rides the servo path, but it is a plain GPIO output driving a
     * transistor gate - no pulse, no angle. `_hasLamp` is false on every board
     * whose profile does not carry it, which is all of them but board 1. */
    PinId _lampPad{};
    bool  _hasLamp = false;

    PWMDriver _servo[PWM_COUNT] = {
    	PWMDriver(pwmConfigOf(ConnType::Pwm0), SERVO_ZERO_PULSE_US, slotHolds(ConnType::Pwm0, DeviceType::Servo)),
		PWMDriver(pwmConfigOf(ConnType::Pwm1), SERVO_ZERO_PULSE_US, slotHolds(ConnType::Pwm1, DeviceType::Servo)),
    	PWMDriver(pwmConfigOf(ConnType::Pwm2), SERVO_ZERO_PULSE_US, slotHolds(ConnType::Pwm2, DeviceType::Servo)),
    	PWMDriver(pwmConfigOf(ConnType::Pwm3), SERVO_ZERO_PULSE_US, slotHolds(ConnType::Pwm3, DeviceType::Servo)),
    };
};



/**
 * A couple names two DIFFERENT servos, and no servo is in two couples.
 *
 * Both halves are cheap to state and impossible to see by eye once the table
 * grows: a row coupling a servo to itself would mirror it against its own
 * command, and a servo in two couples would take two conflicting angles from
 * one request, last write winning.
 */
constexpr bool couplesWellFormed() {
    for (std::size_t i = 0; i < sizeof(SERVO_COUPLES) / sizeof(SERVO_COUPLES[0]); ++i) {
        const ServoCouple& c = SERVO_COUPLES[i];
        if (c.a == c.b) return false;
        for (std::size_t j = i + 1; j < sizeof(SERVO_COUPLES) / sizeof(SERVO_COUPLES[0]); ++j) {
            const ServoCouple& d = SERVO_COUPLES[j];
            if (c.a == d.a || c.a == d.b || c.b == d.a || c.b == d.b) return false;
        }
    }
    return true;
}
static_assert(couplesWellFormed(),
              "a servo couple names one servo twice, or a servo appears in two couples");

/**
 * Both halves of a couple sit on the SAME board, or neither does.
 *
 * ServoThread applies a coupled command to both members from one request, which
 * it can only do for servos it owns. Split the pair across two masters and the
 * command reaches one leaf and not the other - the exact asymmetry the coupling
 * exists to prevent, and on a two-leaf mechanism that is a bind rather than a
 * cosmetic fault.
 *
 * Sound because noDuplicateIds(Servo) above already puts each id on exactly one
 * board: finding one member here and not the other means the other is elsewhere
 * or nowhere, and both are wrong. A couple no board carries yet - ids allocated
 * ahead of the hardware - passes, since neither half is found anywhere.
 */
constexpr bool couplesShareABoard() {
    for (const ServoCouple& c : SERVO_COUPLES)
        for (const BoardProfile& p : PROFILES) {
            bool hasA = false, hasB = false;
            for (const Slot& s : p.slots) {
                if (s.device != DeviceType::Servo) continue;
                if (s.id == c.a) hasA = true;
                if (s.id == c.b) hasB = true;
            }
            if (hasA != hasB) return false;
        }
    return true;
}
static_assert(couplesShareABoard(),
              "a coupled servo pair is split across two boards: one leaf would move without the other");
