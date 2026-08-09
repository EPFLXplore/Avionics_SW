/*
 * PWMDriver.h  --  one 50 Hz PWM output, plus the angle map a servo needs.
 *
 * The channel's wiring is NOT here: it is PwmMux in Pins.h, because Pwm1 is a
 * servo output on one board and the WS2812 strip's DMA channel on another, and
 * both drivers need the same timer and channel. What is here is the part that is
 * about driving a servo - the time base, the angle map, the home position.
 */

#pragma once

#include <cstdint>

#include "Pins.h"   // PwmMux, pwmMux(), portOf/configPin, PROFILES

/* ---- the angle map -------------------------------------------------------- */

/**
 * Servo travel limits: 0 deg is PULSE_MIN_US, ANGLE_MAX_DEG is PULSE_MAX_US.
 * The single source of truth for the angle<->pulse map; change them and both
 * setAngle() and every angleToPulseUs() call follow.
 */
inline constexpr uint16_t PULSE_MIN_US  = 500;
inline constexpr uint16_t PULSE_MAX_US  = 2500;
inline constexpr float    ANGLE_MAX_DEG = 180.0f;

/**
 * @brief Angle in degrees -> pulse width in us, clamped to 0..ANGLE_MAX_DEG.
 *
 * constexpr so a home position can be stated as an angle rather than a
 * hand-computed microsecond count, at zero runtime cost. setAngle() calls the
 * same function, so a commanded angle and a configured one cannot disagree.
 */
constexpr uint16_t angleToPulseUs(float angle)
{
    return angle <= 0.0f          ? PULSE_MIN_US
         : angle >= ANGLE_MAX_DEG ? PULSE_MAX_US
         : static_cast<uint16_t>(PULSE_MIN_US +
                                 (angle / ANGLE_MAX_DEG) * (PULSE_MAX_US - PULSE_MIN_US));
}

/**
 * Home angle for every servo, in degrees. 0 deg is one end of travel, i.e.
 * PULSE_MIN_US - every channel drives a positional servo, so this is a commanded
 * position and nothing re-asserts it behind a command.
 *
 * FALLBACK ONLY. servo_cal.yaml on the RPi is the source of truth: Nexus replays
 * a home angle per servo over ServoRequest{change_zero} on every link-up, and
 * setZeroPulseUs() overwrites this. The compiled value is what a board runs
 * before the first replay - after a reset, a reflash, or with no RPi attached -
 * so it has to be safe rather than right. The same split applies to the mass
 * slopes in MassThread.h and the pH calibration in pHMeterThread.h.
 */
inline constexpr float    SERVO_ZERO_DEG      = 0.0f;
inline constexpr uint16_t SERVO_ZERO_PULSE_US = angleToPulseUs(SERVO_ZERO_DEG);


/* ---- the driver ----------------------------------------------------------- */

class PWMDriver {
public:
    /**
     * @param mux            the slot's wiring, from pwmMux(ConnType::PwmN).
     * @param zeroPulseUs  home position this channel drives on zero().
     * @param enabled        false constructs the channel inert: it touches
     *                       neither pad nor timer, and every setter no-ops. That
     *                       is what keeps a timer clear for another subsystem on
     *                       a board whose profile declares no servo on this slot.
     */
    PWMDriver(const PwmMux& mux, uint16_t zeroPulseUs, bool enabled);
    ~PWMDriver();

    /** Pulse width in microseconds, clamped to the timer period. */
    void setPulseUs(uint16_t us);

    /**
     * @brief Set servo angle in degrees. Clamped and mapped by
     *        angleToPulseUs() (0-ANGLE_MAX_DEG -> PULSE_MIN_US-PULSE_MAX_US).
     */
    void setAngle(float angle);

    /** Drive the configured home position. */
    void zero();

    /** Move the home position. Nexus replays this from servo_cal.yaml on every
     *  link-up, the same way it replays mass slopes and the pH calibration. The
     *  MCU keeps it in RAM only, so a reset drops back to SERVO_ZERO_PULSE_US. */
    void setZeroPulseUs(uint16_t us) { _zeroPulseUs = us; }

    /**
     * Global device id this channel drives, bound in ServoThread::init() from
     * the board profile, or NO_DEVICE when this board has nothing on the pad.
     * The driver never reads it - it is identity for the thread to match
     * incoming requests against, kept here so a device carries its own name.
     */
    uint8_t globalId = NO_DEVICE;

private:
    void set50Hz();
    void disableCcrPreload();
    uint32_t timerClock() const;

    PwmMux   _mux;
    uint16_t _zeroPulseUs;
    uint32_t _channelHal;
    bool     _enabled;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////--CHECKS--DOWN--///////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/* Proves the map really is compile-time evaluable, and pins it: a static_assert
 * can only use a constant expression, so this fails to build if it ever isn't. */
static_assert(SERVO_ZERO_PULSE_US      == PULSE_MIN_US, "servo zero drifted from 0 deg");
static_assert(angleToPulseUs(0.0f)  == PULSE_MIN_US, "0 deg must map to PULSE_MIN_US");
static_assert(angleToPulseUs(180.0f)== PULSE_MAX_US, "180 deg must map to PULSE_MAX_US");


/** Every channel drives a real timer channel. */
constexpr bool pwmChannelsValid() {
    for (const PwmMux& m : PWM_MUX)
        if (m.timer == TimerId::None || m.channel < 1 || m.channel > 4) return false;
    return true;
}
static_assert(pwmChannelsValid(), "a PWM channel has no timer, or is outside 1..4");

/**
 * A strip and a servo cannot share a timer's time base.
 *
 * PSC and ARR are per timer, so the strip's 800 kHz bit period and a servo's
 * 50 Hz base cannot both be programmed into one, whichever channels they use.
 * The only rule that spans slots, and it lives here because it needs both the
 * mux (which slots share a timer) and the profile (what is on them).
 */
constexpr bool noTimerBaseConflict() {
    for (const BoardProfile& p : PROFILES)
        for (uint8_t a = 0; a < PWM_COUNT; ++a)
            for (uint8_t b = a + 1; b < PWM_COUNT; ++b) {
                if (PWM_MUX[a].timer != PWM_MUX[b].timer) continue;
                const DeviceType da = p.slots[PWM_FIRST + a].device;
                const DeviceType db = p.slots[PWM_FIRST + b].device;
                if ((da == DeviceType::LedStrip && db == DeviceType::Servo) ||
                    (db == DeviceType::LedStrip && da == DeviceType::Servo)) return false;
            }
    return true;
}
static_assert(noTimerBaseConflict(),
              "a board puts a servo and the strip on channels of the same timer");
