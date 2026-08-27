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
{}

pHMeterThread::~pHMeterThread(){}

void pHMeterThread::init(){
    _meter.adc.begin();
    _meter.probe = _meter.adc.probe();   // debugger-visible wiring verdict
}

void pHMeterThread::loop(){

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

    /* Moving average over VOLTS, calibration applied after the mean.
     *
     * Averaging calibrated pH instead is equivalent only while the calibration
     * holds still across the window - and the window is never cleared, so a new
     * slope/offset mid-run blends two coordinate systems for PH_AVG_SIZE samples
     * (6.25 s at ~1.6 Hz). calibrate_ph.sh hits this every run: it switches to
     * the identity to read raw volts, and the blend published ~0.89 - neither a
     * voltage nor a pH - long enough for the script's own range check to call
     * the calibration unapplied when it had been applied.
     *
     * Volts carry no calibration, so a new pair applies to the whole history at
     * once and the next published value is already right. Otherwise identical:
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
