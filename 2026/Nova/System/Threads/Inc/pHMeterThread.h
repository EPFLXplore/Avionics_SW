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

#include "cmsis_os2.h"

/** Moving-average depth. A pH probe is a very high impedance source and the
 *  reading wanders; averaging ~10 samples at 2 Hz is 5 s of smoothing, which is
 *  fast next to how quickly the solution being measured actually changes. */
constexpr uint8_t PH_AVG_SIZE = 10;

struct PhType {
    ADS1114 adc;                       // each _meter owns its front end (no pointers)

    /* Two-point calibration: ph = slope * volts + offset, where `volts` is the
     * DIFFERENTIAL reading AINP - AINN.
     *
     * On the pH hat that is the glass electrode's raw EMF. Both LTC2064 halves
     * are unity-gain followers (OUTA tied to -INA, OUTB to -INB), AINP follows
     * the BNC centre and AINN follows the BNC shell, so the gain is 1 and the
     * VDD/2 bias on the shell (R1/R2) is pure common mode that the differential
     * measurement rejects. Nothing scales the electrode.
     *
     * The fallbacks are therefore the ideal Nernst response at 25 C: 59.16 mV
     * per pH with 0 V at pH 7, i.e. slope = -1/0.05916 = -16.904 pH/V and
     * offset = 7. The sign is negative because electrode potential FALLS as pH
     * rises.
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
    float slope  = -16.904f;           // pH per volt  (= -1 / 0.05916)
    float offset = 7.0f;               // pH at 0 V differential

    float volts  = 0.0f;               // last raw differential reading [V]
    float ph     = 0.0f;               // last averaged, calibrated pH
    float window[PH_AVG_SIZE] = {};    // moving-average ring
    uint8_t filled = 0;                // samples in `window` so far (< PH_AVG_SIZE while warming up)

    uint8_t probe = 0;                 // ADS1114::probe() verdict, debugger-visible
    uint16_t errors = 0;               // bus/timeout failures since boot

    explicit PhType(const ADS1114& frontEnd) : adc(frontEnd) {}
};

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
     *
     * Those pads carry .ioc labels from the OTHER use of this connector - a
     * bit-banged HX711, where PC8 is the clock and PC9 the data line. One
     * connector, two mutually exclusive personalities; the slot's DeviceType tag
     * picks which, so both can never be claimed at once. */
    PhType _meter{ADS1114{pinOf(ConnType::ConnI2C),
                         BusId::I2c3, 8,             // I2C3 on these pads is AF8
                         ADS1114::ADDR_VDD,          // JP1 bridges A-C: ADDR to VDD
                         ADS1114::FsrType::V0_512,   // +-414 mV electrode span
                         ADS1114::RateType::SPS8}};

    void publish(PhType& device);
};

