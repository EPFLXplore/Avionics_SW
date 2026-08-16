/*
 * TestTask.cpp
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#include <MassThread.h>

MassThread::MassThread(const char* name, osPriority priority)
: MessageThread(name, priority)
{
	// _cell[] own their HX711 (constructed in the header) - nothing to wire.
	// The board profile is NOT read here: see init().
}

MassThread::~MassThread(){
	// _cell[] are statically allocated members: nothing to free.
}

void MassThread::init(){
	// Bind local connectors to global device ids. HERE, not in the constructor:
	// init() only runs because Thread::start() was called, which only happened
	// because System asked hasDevices() - and hasDevices() answers off the
	// profile directly, so it never needed the binding. Keeping the read in task
	// context means nothing about the board identity is latched before the
	// scheduler is up.
	//
	// This is the ONLY place the board profile is bound; everything else works
	// off _cell[i].globalId, so swapping this for hardware detection later
	// changes nothing beyond this one line.
	bindSlots(_cell, DeviceType::LoadCell, ConnType::ConnI2C);

	// EVERY loop below skips unbound connectors, the same test loop() publishes
	// under. A cell whose slot holds something else must touch nothing at all:
	// HX711::begin() claims the pair as bit-banged GPIO, so running it on a
	// connector carrying the pH probe would tear I2C3 out from under ADS1114 -
	// silently, because nothing here would report an error. This is the same
	// rule PWMDriver's `enabled` flag enforces for a servo channel; a load cell
	// just expresses it through globalId, which bindSlots has already set.
	//
	// It went unnoticed while the only mass-carrying board had a cell on both
	// connectors, where the guard was a no-op.
	for (MassType& c : _cell)
		if (c.globalId != NO_DEVICE) c.hx.begin();

	// Settling from power-up is 50 ms at RATE=1 / 80 SPS (Table 2, "output
	// settling time"; it is 400 ms at RATE=0). Nothing power-cycles the chip here
	// any more, and taskRun() already burned one thread period plus MCU boot
	// before init() ran, so most of that window is gone - 100 ms is slack.
	//
	// It does NOT cover the strain gauges' thermal settling, which is seconds. A
	// boot tare taken during that transient now fails tareValid instead of being
	// adopted silently - see tareScale().
	osDelay(100);
	for (MassType& c : _cell)
		if (c.globalId != NO_DEVICE) this->tareScale(c);
}

void MassThread::loop(){
    // Always pop, so the queue drains even for devices this board does not have.
    MassRequest cmd;
    if (this->popCommand(cmd)) {
    	if (MassType* dev = deviceFor(_cell, cmd.id)) {
    		if (cmd.change_scale) {
    			dev->slope = cmd.scale; // runtime calibration override
    		}
    		if (cmd.tare) {
    			this->tareScale(*dev);
    		}
    	}
    }

    // Publish only bound cells, so a global id means one physical scale fleet-wide.
    for (MassType& c : _cell)
    	if (c.globalId != NO_DEVICE) this->updateMass(c);
}

void MassThread::updateMass(MassType& device){
	this->update(device);

	// Sample every loop (100 Hz, to keep up with the chip at 80 SPS), publish
	// every PUBLISH_EVERY-th (~10 Hz, the rate the link and the calibration
	// script were built around). See PUBLISH_EVERY in MassThread.h.
	if (++device.pubTick < PUBLISH_EVERY) return;
	device.pubTick = 0;

	MassPacket st;
	st.mass = device.weight;
	st.id = device.globalId;
    pushStatus(st);
}


void MassThread::shift(float *array , int N, float valueIn){  //shifts all array values left and adds valueIn at position N-1
  for(int i = 1; i<N ; i++){
    array[i-1] = array[i];
  }
  array[N-1] = valueIn;
}

float MassThread::movingAverage(const float *arr, uint8_t n) {
  if(n<=0){return 0;}
  float sum = 0.0f;
  for (uint8_t i = 0; i < n; i++) sum += arr[i];
  return sum / n;
}

void MassThread::update(MassType& device)
{
	if (!device.hx.available()) { device.nNotReady++; return; }

	volatile int32_t raw = 0;
	switch (device.hx.read(raw)) {
	    case HX711::ReadResultType::Timeout:    device.nTimeout++;    return;
	    case HX711::ReadResultType::ClockFault: device.nClockFault++; return;
	    case HX711::ReadResultType::Ok:         break;
	}
	device.nGood++;
	device.lastRaw = raw;

    // 1. Shift and average
    this->shift(device.buffer, AVG_SIZE, (float)raw);
    float avg = this->movingAverage(device.buffer, AVG_SIZE);

    // 2. Apply offset AND slope
    // Result = (Current - Zero) * CalibrationFactor
   float val = (avg - device.offset) * device.slope;

   device.weight = val;
}

void MassThread::tareScale(MassType& device) {
	// Average up to TARE_SAMPLES fresh samples. read() blocks (bounded) until
	// each one is ready, so no available() pre-check: the old guard silently
	// SKIPPED the tare whenever the chip wasn't ready yet (it rarely is 110 ms
	// after power-up), leaving offset = 0.
	//
	// The result is only adopted if it clears BOTH gates below. Refusing a tare
	// keeps the previous offset, which at boot is 0.0f - tareValid is what tells
	// you which of those you are looking at.
	int64_t sum      = 0;
	uint8_t good     = 0;
	uint8_t timeouts = 0;
	int32_t lo       = INT32_MAX;
	int32_t hi       = INT32_MIN;
	bool    abort    = false;

	for (uint8_t i = 0; i < TARE_SAMPLES && !abort; ++i) {
		int32_t raw = 0;
		switch (device.hx.read(raw)) {
		case HX711::ReadResultType::Ok:
			sum += raw; ++good;
			if (raw < lo) lo = raw;
			if (raw > hi) hi = raw;
			break;

		case HX711::ReadResultType::ClockFault:
			// Was silently skipped here: neither counted nor breaking, and the
			// counters are only touched in update(). A tare taken over a
			// marginal SCK line was indistinguishable from a clean one.
			++device.nClockFault;
			break;

		case HX711::ReadResultType::Timeout:
			// Was a first-strike break, so ONE late conversion ended the tare
			// with however few samples it had already collected - the shortest
			// path to a one-sample zero. A chip reset costs 400 ms of settling
			// (Table 2) and surfaces exactly here, which is how a sagging rail
			// reaches the offset. Spend a small budget before giving up.
			++device.nTimeout;
			if (++timeouts >= TARE_MAX_TIMEOUTS) abort = true;
			break;
		}
	}

	device.tareGood   = good;
	device.tareSpread = (good > 0) ? (hi - lo) : 0;

	// Too few survivors: the sqrt(N) noise reduction the average is there to buy
	// is largely gone, and at good == 1 the zero is a single sample - the 2025
	// behaviour this routine was rewritten to leave behind. Keep the old offset.
	if (good < TARE_MIN_GOOD) { device.tareValid = false; return; }

	// Well-formed samples that disagree with each other: EMI corrupting a bit on
	// DOUT, a chip still settling, or the cell genuinely moving. read() validates
	// the clock train and never the data, so this is the only place any of that
	// can be caught.
	if ((hi - lo) > TARE_MAX_SPREAD) { device.tareValid = false; return; }

	device.offset    = (float)(sum / good);
	device.tareValid = true;

	for (uint8_t i = 0; i < AVG_SIZE; ++i) {
		device.buffer[i] = device.offset;
	}
}

/*
void MassThread::sendfloat(float value) {
	snprintf(this->_buffer, sizeof(buffer), "Mass value = %.3f\r\n", value);
	CDC_Transmit_FS((uint8_t*)buffer, sizeof(buffer));
}

void MassThread::sendint(int32_t value) {
	snprintf(this->_buffer, sizeof(buffer), "Mass value = %d\r\n", value);
	CDC_Transmit_FS((uint8_t*)buffer, sizeof(buffer));
}
*/


