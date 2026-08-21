/*
 * pHMeterThread.cpp  --  see pHMeterThread.h.
 */

#include <pHMeterThread.h>

namespace {
/* How far short of the nominal conversion time the pre-sleep stops. The
 * datasheet allows +-10% on the data rate, which is 12.5 ms at 8 SPS - so this
 * is NOT a tolerance on the conversion finishing early, it is just the point
 * where handing the rest to the poll loop is cheaper than sleeping again. */
constexpr uint32_t PRE_SLEEP_MARGIN_MS = 5;
} // namespace

pHMeterThread::pHMeterThread(const char* name, osPriority priority)
: MessageThread(name, priority)
{
    // _meter owns its ADS1114 (constructed in the header) - nothing to wire.
    // The board profile is NOT read here: see init().
}

pHMeterThread::~pHMeterThread(){
    // _meter is a statically allocated member: nothing to free.
}

void pHMeterThread::init(){
    // Only reached because System asked hasDevices() and got true, so a probe is
    // fitted. Bringing the bus up on a board without one would claim PC8/PC9 as
    // I2C3 AF and take the pins out from under a bit-banged HX711.
    _meter.adc.begin();
    _meter.probe = _meter.adc.probe();   // debugger-visible wiring verdict
}

void pHMeterThread::loop(){
    // Always pop, so the queue drains even on a board with no probe fitted - but
    // the pop's verdict GATES the read. xQueueReceive leaves `cmd` untouched when
    // the queue is empty, which is almost every tick, so reading it unconditionally
    // reads stale stack: a nonzero change_cal byte there overwrites slope/offset
    // with whatever floats the previous iteration left behind, and those persist in
    // _meter for the rest of the run. Same shape as MassThread::loop().
    //
    // Drained to EMPTY, not one per tick: this loop runs at 500 ms, so a
    // one-per-tick drain gives a queued command up to half a second of
    // head-of-line delay for every entry ahead of it. Same reasoning as
    // MassThread::loop(), and it matters more here because the tick is 50x
    // longer. A calibration is idempotent, so applying a whole backlog in one
    // tick just lands on the newest one.
    PhRequest cmd{};
    while (this->popCommand(cmd)) {
        if (cmd.change_cal) {
            // No id to match on: the command names the one probe this rover has.
            _meter.slope  = cmd.slope;   // runtime calibration override
            _meter.offset = cmd.offset;
        }
    }

    this->sample(_meter);
    this->publish(_meter);
}

/*
 * One polled, single-shot measurement.
 *
 * The ADS1114 sits powered down until we ask, so the sequence is: write the
 * config with OS = 1, sleep through the bulk of the conversion, then poll the
 * same bit back until it lands. Every wait is osDelay() rather than a spin, so
 * this thread costs the CPU nothing while it runs - which matters because it
 * shares a priority with the load cells.
 *
 * Sleeping first rather than polling the whole way is worth it because the
 * duration is KNOWN from the data rate: waking 125 times at 8 SPS to ask "done
 * yet?" is 125 context switches and 125 two-byte I2C register reads per sample,
 * for no information - the answer is no until it is not. Two wakeups do the same
 * job.
 *
 * The budget is 2x the nominal conversion time plus a few ms: the datasheet
 * allows +-10% on the data rate, and a chip that has stopped converting must
 * not wedge the loop forever. The pre-sleep counts against it, so a wedged chip
 * still times out at the same wall-clock point it used to.
 */
void pHMeterThread::sample(PhType& device){
    if (device.adc.startConversion() != ADS1114::ResultType::Ok) {
        ++device.errors;
        return;   // keep the last good reading rather than publishing a lie
    }

    const uint32_t conv   = device.adc.conversionMs();
    const uint32_t budget = conv * 2 + 5;
    uint32_t waited = 0;

    /* Guarded, and the guard is load-bearing: conversionMs() is 2-4 ms at the top
     * data rates, where conv - PRE_SLEEP_MARGIN_MS would wrap unsigned into an
     * osDelay of ~49 days and hang the thread forever. At those rates the whole
     * conversion is shorter than the margin anyway, so polling immediately is
     * both correct and already cheap. */
    if (conv > PRE_SLEEP_MARGIN_MS) {
        waited = conv - PRE_SLEEP_MARGIN_MS;
        osDelay(waited);
    }

    while (!device.adc.ready()) {
        if (waited >= budget) { ++device.errors; return; }
        osDelay(1);
        ++waited;
    }

    float volts = 0.0f;
    if (device.adc.readVolts(volts) != ADS1114::ResultType::Ok) {
        ++device.errors;
        return;
    }
    device.volts = volts;

    /* Moving average over VOLTS, with the calibration applied after the mean.
     *
     * This used to average the calibrated pH, on the reasoning that the map is
     * affine so the two are equivalent. They are - but only while the
     * calibration is CONSTANT across the window, which is exactly when it is
     * not. The window is never cleared, so a calibration arriving mid-run left
     * the ring holding values computed under the OLD slope/offset while new ones
     * came in under the new pair, and the mean of those is not a pH under either
     * calibration. For PH_AVG_SIZE samples (6.25 s at ~1.6 Hz) the topic then
     * published a blend of two coordinate systems.
     *
     * That is not academic: calibrate_ph.sh switches to the identity (slope 1,
     * offset 0) to read raw volts, and the blend made the topic report ~0.89 -
     * neither a voltage nor a pH - long enough for the script's own range check
     * to declare the calibration had not been applied when it had.
     *
     * Averaging the raw quantity fixes it at the root: the window holds volts,
     * which mean nothing calibration-dependent, so a new slope/offset applies to
     * the whole history at once and the very next published value is correct.
     * No flush, no blend, no transient. Same arithmetic otherwise - for a fixed
     * calibration this is bit-for-bit the old behaviour, since
     * mean(m*v + b) == m*mean(v) + b. */
    for (uint8_t i = PH_AVG_SIZE - 1; i > 0; --i)
        device.window[i] = device.window[i - 1];
    device.window[0] = volts;
    if (device.filled < PH_AVG_SIZE) ++device.filled;

    float sum = 0.0f;
    for (uint8_t i = 0; i < device.filled; ++i) sum += device.window[i];
    device.ph = device.slope * (sum / static_cast<float>(device.filled)) + device.offset;
}

void pHMeterThread::publish(PhType& device){
    // Only the finished value goes out: the MCU owns the volts -> pH conversion,
    // so there is one place it happens and the RPi has nothing to recompute.
    PhPacket st;
    st.ph = device.ph;
    pushStatus(st);
}
