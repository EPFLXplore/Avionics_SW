/*
 * pHMeterThread.cpp  --  see pHMeterThread.h.
 */

#include <pHMeterThread.h>

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
    // Always pop, so the queue drains even on a board with no probe fitted.
    PhRequest cmd;
    if (this->popCommand(cmd) && cmd.change_cal) {
        // No id to match on: the command names the one probe this rover has.
        _meter.slope  = cmd.slope;   // runtime calibration override
        _meter.offset = cmd.offset;
    }

    this->sample(_meter);
    this->publish(_meter);
}

/*
 * One polled, single-shot measurement.
 *
 * The ADS1114 sits powered down until we ask, so the sequence is: write the
 * config with OS = 1, then poll the same bit back until the conversion lands.
 * The wait is osDelay(1) per poll rather than a spin - a 125 ms conversion at
 * 8 SPS would otherwise burn a whole tick budget doing nothing, and this thread
 * runs at the same priority as the load cells.
 *
 * The budget is 2x the nominal conversion time plus a few ms: the datasheet
 * allows +-10% on the data rate, and a chip that has stopped converting must
 * not wedge the loop forever.
 */
void pHMeterThread::sample(PhType& device){
    if (device.adc.startConversion() != ADS1114::ResultType::Ok) {
        ++device.errors;
        return;   // keep the last good reading rather than publishing a lie
    }

    const uint32_t budget = device.adc.conversionMs() * 2 + 5;
    uint32_t waited = 0;
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

    // Moving average over the calibrated value. Averaging pH rather than volts
    // is equivalent here (the map is affine) and keeps the window meaningful if
    // a calibration arrives mid-run.
    const float ph = device.slope * volts + device.offset;

    for (uint8_t i = PH_AVG_SIZE - 1; i > 0; --i)
        device.window[i] = device.window[i - 1];
    device.window[0] = ph;
    if (device.filled < PH_AVG_SIZE) ++device.filled;

    float sum = 0.0f;
    for (uint8_t i = 0; i < device.filled; ++i) sum += device.window[i];
    device.ph = sum / static_cast<float>(device.filled);
}

void pHMeterThread::publish(PhType& device){
    // Only the finished value goes out: the MCU owns the volts -> pH conversion,
    // so there is one place it happens and the RPi has nothing to recompute.
    PhPacket st;
    st.ph = device.ph;
    pushStatus(st);
}
