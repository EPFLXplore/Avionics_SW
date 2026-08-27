/*
 * pHMeterThread.h  --  solution pH over an ADS1114 on the I2C connector.
 *
 * Shaped like MassThread - an array indexed by CONNECTOR, bound from the board
 * profile in init() - but the probe is not addressable: PhPacket carries only
 * the finished pH value, no id, because there is one probe and only ever will
 * be. The MCU does the conversion; the RPi just publishes what it is told.
 *
 * Front end (hats/2026/phmeter): BNC electrode -> 1M/10nF input filter -> dual
 * unity-gain LTC2064 followers -> 1k/10nF RC into the ADS1114's differential
 * pair. AINP is the BNC centre, AINN the BNC shell held at VDD/2, so the ADC
 * reads the electrode EMF directly with no gain: +-414 mV covers pH 0..14, which
 * is why the PGA is set to +-0.512 V rather than the driver's wider default.
 * That puts one LSB at 15.6 uV, about 0.00026 pH - far below the probe's own
 * noise, which is the right place for it to be.
 *
 * Sampling is polled and one-shot, driven from loop(): each tick starts one
 * conversion, then polls the ADS1114's OS bit with osDelay() in between until it
 * completes. The chip powers down between conversions, so nothing runs unless
 * this thread asks - and because the waiting is osDelay() rather than a spin,
 * a 125 ms conversion at 8 SPS costs the CPU nothing.
 */

#pragma once
#include "MessageThread.h"
#include "ADS1114.h"
#include "packets.h"
#include "BoardProfile.h"  // which connector carries the probe, per board
#include "Pins.h"          // pinOf(), BusId: that connector's pads and bus

#include "cmsis_os2.h"

/** Moving-average depth. A pH probe is a very high impedance source and the
 *  reading wanders. The loop runs at 500 ms (System.cpp) plus a 125 ms conversion
 *  at 8 SPS, so ~1.6 Hz: 10 samples is a 6.25 s window, fast next to how quickly
 *  the solution being measured actually changes.
 *
 *  What it buys, measured on this probe: raw noise is ~0.0045 pH peak-to-peak, so
 *  averaging 10 gives sqrt(10) ~ 3.2x, about 0.0015 pH. What it costs is group
 *  delay of (N-1)/2 = 4.5 samples, ~2.8 s. It does NOT delay equilibrium - the
 *  electrode settles when it settles - only your view of it.
 *
 *  SET TO 1 TO DISABLE AVERAGING and publish every conversion raw - the whole
 *  filter collapses to a copy: the shift loop's bound is PH_AVG_SIZE - 1, which
 *  promotes to int 0 and never runs, `filled` saturates at 1, and the mean of one
 *  sample is that sample.
 *
 *  Currently 10 (flight setting) - see the static_assert below for why 0 is not
 *  the way to ask for no averaging.
 *
 *  Use 1 when CHARACTERISING a probe rather than flying it. Unfiltered samples
 *  are what let you watch a settling transient decay and extrapolate where it is
 *  heading - on 2026-08-12 a 6.86 buffer stepped 7.44 -> 9.73 -> 10.30 mV, and
 *  the ratio of those deltas said it had converged to ~10.5 without waiting
 *  another hour. A 10-deep window smears exactly that structure into a smooth
 *  curve with no usable deltas. Raw to characterise, averaged to fly.
 *
 *  Whichever it is, scripts/calibrate_ph.sh in Avionics_ROS assumes it: its
 *  steadiness gate reads consecutive published samples, and averaged ones are
 *  correlated (they share 9 of 10 window entries), so max-min understates real
 *  movement. SETTLE_S=60 there covers both the 6.25 s window flush and the
 *  electrode transient, so 10 is safe - but do not shorten it. */
constexpr uint8_t PH_AVG_SIZE = 10;


struct PhType {
    ADS1114 adc;                       // each _meter owns its front end (no pointers)

    float slope  = -18.736416f;        // pH per volt  (ideal would be -16.904)
    float offset =   5.452840f;        // pH at 0 V differential (ideal 7.0)

    float volts  = 0.0f;               // last raw differential reading [V]
    float ph     = 0.0f;               // last averaged, calibrated pH
    float window[PH_AVG_SIZE] = {};    // moving-average ring, in VOLTS (see .cpp:
                                       // averaging the raw quantity keeps the
                                       // window valid across a calibration change)
    uint8_t filled = 0;                // samples in `window` so far (< PH_AVG_SIZE while warming up)

    uint8_t probe = 0;                 // ADS1114::probe() verdict, debugger-visible
    uint16_t errors = 0;               // bus/timeout failures since boot

    explicit PhType(const ADS1114& frontEnd) : adc(frontEnd) {}
};



/* Two-PARAMETER calibration: ph = slope * volts + offset, where `volts` is
 * the DIFFERENTIAL reading AINP - AINN. Two parameters, but fitted from
 * three buffers: two points always fit a line perfectly, so the third
 * point's residual is the only thing that can tell you the electrode has
 * gone lazy. See scripts/calibrate_ph.sh in Avionics_ROS.
 *
 * On the pH hat that is the glass electrode's raw EMF. Both LTC2064 halves
 * are unity-gain followers (OUTA tied to -INA, OUTB to -INB), AINP follows
 * the BNC centre and AINN follows the BNC shell, so the gain is 1 and the
 * VDD/2 bias on the shell (R1/R2) is pure common mode that the differential
 * measurement rejects. Nothing scales the electrode.
 *
 * The ideal Nernst response at 25 C is 59.16 mV per pH with 0 V at pH 7,
 * i.e. slope = -1/0.05916 = -16.904 pH/V and offset = 7. The sign is
 * negative because electrode potential FALLS as pH rises. That is the SHAPE
 * these numbers must keep; sanity-check any new pair against it.
 *
 * The values below are MEASURED rather than ideal - the installed electrode,
 * fitted 2026-08-13 across 4.01 / 6.86 / 9.18: 57.41 mV/pH (97.0% of
 * theoretical) with a +7.4 mV asymmetry potential, worst residual 0.0056 pH,
 * the tightest of the three fits so far. A real calibration beats the ideal
 * line for as long as THIS electrode is the one on the BNC. Swap the probe
 * and they are wrong in a way nothing detects: put -16.904f / 7.0f back
 * until the new one has been calibrated.
 *
 * Third fit on this electrode, and the sequence is worth more than any one
 * of them:
 *
 *   2026-08-12 early  -17.460922f / 6.891526f  57.27 mV/pH  96.8%  -6.2 mV
 *   2026-08-12 late   -17.265400f / 7.053987f  57.92 mV/pH  97.9%  +3.1 mV
 *   2026-08-13 below  -17.417130f / 7.128631f  57.41 mV/pH  97.0%  +7.4 mV
 *
 * Nothing was wrong with the early runs: the probe had been in solution less
 * than an hour and its gel layer was still forming, which suppresses the
 * slope and shifts E0. Read the last two columns separately though, because
 * they no longer say the same thing. The RESPONSE has stopped moving -
 * 97.9% then 97.0%, a step a three-point fit cannot resolve - so the gel
 * layer is done. The ASYMMETRY has not: -6.2 -> +3.1 -> +7.4 mV, still
 * walking one way. Slope agreeing while offset does not is the signature
 * described below, and here it means E0 is settling NOW: expect the offset
 * to keep moving and plan to refit, rather than treating this pair as final.
 * It is not decay - a dying electrode loses slope, and this one has held it.
 *
 * The lesson is in the timing, not the numbers: a probe that has been stored
 * dry needs HOURS of soaking before it will hold an offset. Calibrating
 * sooner produces a clean-looking fit with small residuals - the linearity
 * is real, the electrode is simply on a different line than it will be on
 * tonight - so no residual check can catch it. The tell is a later fit whose
 * slope agrees while its offset does not. Soak first.
 *
 * FALLBACK ONLY. ph_cal.yaml on the RPi is the source of truth, replayed
 * over PhRequest on every link-up; these are what the board runs before the
 * first replay - after a reset, a reflash, or with no RPi attached.
 *
 * Two caveats these numbers cannot cover, and why calibration is not
 * optional: a real electrode's slope degrades with age (90-105% of
 * theoretical) and carries an offset of tens of mV, and the Nernst slope is
 * proportional to absolute temperature (0.1984 * T mV/pH) while this board
 * has no temperature sensor. Nexus replays a measured calibration over
 * PhRequest on every link-up, same path as the mass slopes. */
/* DFRobot probe, fitted 2026-08-20. A different electrode from the three
 * fits tabulated above - those belong to the previous probe and are kept
 * only as a record of how a calibration series reads. Do not compare across
 * the swap; asymmetry potential and slope are per-electrode.
 *
 *   53.37 mV/pH = 90.2% of theoretical, -82.6 mV asymmetry at pH 7
 *
 * Both numbers are marginal and flagged for re-check: 90.2% clears the
 * script's "good" band by 0.2%, and -82.6 mV is ~2.8x outside the +-30 mV
 * a healthy glass electrode holds. A resistive leak on the input measured
 * the same day (14.7 GOhm open-circuit, falling to 2-4 GOhm) attenuates the
 * slope and drags the offset negative together - exactly this shape. Refit
 * with a dry BNC before trusting these. */

class pHMeterThread : public MessageThread<PhRequest, PhPacket> {
public:
    pHMeterThread(const char* name, osPriority priority);
    ~pHMeterThread();

    /** True when this board carries at least one pH _meter. Read live off the
     *  profile, so System can ask before anything is bound - same contract as
     *  MassThread::hasDevices(). */
    bool hasDevices() const { return anySlot(DeviceType::PhMeter); }

    /** Debugger handle: the one probe. */
    PhType& probeMeter() { return _meter; }

    void init() override;
    void loop() override;

    /** Start a conversion, poll until it lands, fold it into the average. */
    void sample(PhType& device);

private:
    /* ONE probe, not an array: PhPacket carries no id because the rover has one
     * and only ever will, and deviceFitsSlot() pins it to the connector
     * whose pads reach I2C3. Same shape as LedsThread's single strip.
 */
    PhType _meter{ADS1114{pinOf(ConnType::ConnI2C),
                         BusId::I2c3, 8,             // I2C3 on these pads is AF8
                         ADS1114::ADDR_VDD,          // JP1 bridges A-C: ADDR to VDD
                         ADS1114::FsrType::V0_512,   // +-414 mV electrode span
                         ADS1114::RateType::SPS8}};

    void publish(PhType& device);
};

/* 0 would be a zero-length array and a divide by zero on the first sample, and
 * both would be silent: GCC accepts [0] as an extension, and 0.0f/0.0f is a
 * quiet NaN that would just start appearing on the topic. 1 is "no averaging". */
static_assert(PH_AVG_SIZE >= 1, "PH_AVG_SIZE must be >= 1; use 1 for no averaging");
