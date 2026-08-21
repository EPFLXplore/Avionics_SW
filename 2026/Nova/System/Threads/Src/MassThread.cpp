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
	// adopted silently - see commitTare().
	osDelay(100);
	// Boot tare, run to completion right here. init() is the one place where
	// blocking is the correct behaviour - the thread has no queue to service
	// yet and no packet to publish - so it drives the same state machine loop()
	// drives, just without giving up the CPU between samples. read() still
	// osDelay()s inside, so the rest of the system runs.
	for (MassType& c : _cell) {
		if (c.globalId == NO_DEVICE) continue;
		this->beginTare(c);
		while (c.taring) this->stepTare(c);
	}
}

void MassThread::loop(){
    // Drain the WHOLE queue every tick, not one command per tick. Always pop, so
    // it drains even for devices this board does not have.
    //
    // One-per-tick is what let a backlog outlive the burst that created it: the
    // queue is a FIFO, so a command that lands behind N others waits N ticks to
    // reach the front - and if commands arrive as fast as they are consumed, it
    // never reaches the front at all. That is why a DEEPER queue made commands
    // stop arriving instead of making them more reliable: depth 50 is 50 ticks
    // of head-of-line delay where depth 5 was five. Draining to empty makes the
    // depth a burst allowance again rather than a latency multiplier.
    //
    // Safe to drain unbounded only because arming a tare is now O(1) - see
    // beginTare(). Draining the old blocking tareScale() to empty would have
    // been seconds of frozen thread per tick.
    MassRequest cmd;
    while (this->popCommand(cmd)) {
    	if (MassType* dev = deviceFor(_cell, cmd.id)) {
    		if (cmd.change_scale) {
    			dev->slope = cmd.scale; // runtime calibration override
    		}
    		if (cmd.tare) {
    			this->beginTare(*dev); // arms it; stepTare() does the sampling
    		}
    	}
    }

    // Publish only bound cells, so a global id means one physical scale fleet-wide.
    for (MassType& c : _cell)
    	if (c.globalId != NO_DEVICE) this->updateMass(c);
}

void MassThread::updateMass(MassType& device){
	// A tare in progress owns this cell's conversions: the chip produces one
	// reading per period and update() would otherwise race the tare for it,
	// leaving both averaging half the samples they think they have.
	//
	// Nothing is published for this cell until the tare commits - exactly as
	// before, when the whole thread stopped for the duration - but now it is
	// only this cell, and only for its own conversions. The other cell keeps
	// sampling and publishing, and the command queue keeps draining.
	if (device.taring) { this->stepTare(device); return; }

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

void MassThread::beginTare(MassType& device) {
	// Arm only. Every accumulator starts clean, so a second request landing
	// mid-tare restarts the average rather than poisoning it with samples from
	// both attempts - which is what "tare again" means when the operator sends
	// it because the first one looked wrong.
	device.taring       = true;
	device.tareSum      = 0;
	device.tareTries    = 0;
	device.tareTimeouts = 0;
	device.tareGood     = 0;
	device.tareLo       = INT32_MAX;
	device.tareHi       = INT32_MIN;
}

void MassThread::stepTare(MassType& device) {
	// ONE fresh sample per call. read() blocks (bounded) until it is ready, so
	// no available() pre-check: the old guard silently SKIPPED the tare whenever
	// the chip wasn't ready yet (it rarely is 110 ms after power-up), leaving
	// offset = 0.
	int32_t raw = 0;
	switch (device.hx.read(raw)) {
	case HX711::ReadResultType::Ok:
		device.tareSum += raw;
		++device.tareGood;
		if (raw < device.tareLo) device.tareLo = raw;
		if (raw > device.tareHi) device.tareHi = raw;
		break;

	case HX711::ReadResultType::ClockFault:
		// Was silently skipped: neither counted nor breaking, and the counters
		// are only touched in update(). A tare taken over a marginal SCK line
		// was indistinguishable from a clean one.
		++device.nClockFault;
		break;

	case HX711::ReadResultType::Timeout:
		// Was a first-strike break, so ONE late conversion ended the tare with
		// however few samples it had already collected - the shortest path to a
		// one-sample zero. A chip reset costs 400 ms of settling (Table 2) and
		// surfaces exactly here, which is how a sagging rail reaches the offset.
		// Spend a small budget before giving up.
		++device.nTimeout;
		if (++device.tareTimeouts >= TARE_MAX_TIMEOUTS) { this->commitTare(device); return; }
		break;
	}

	// Budget counts ATTEMPTS, not survivors, exactly as the old for-loop bound
	// did: a cell that faults every read must still finish and be refused by the
	// gates rather than sampling forever.
	if (++device.tareTries >= TARE_SAMPLES) this->commitTare(device);
}

void MassThread::commitTare(MassType& device) {
	// The result is only adopted if it clears BOTH gates below. Refusing a tare
	// keeps the previous offset, which at boot is 0.0f - tareValid is what tells
	// you which of those you are looking at.
	device.taring     = false;
	device.tareSpread = (device.tareGood > 0) ? (device.tareHi - device.tareLo) : 0;

	// Too few survivors: the sqrt(N) noise reduction the average is there to buy
	// is largely gone, and at tareGood == 1 the zero is a single sample - the 2025
	// behaviour this routine was rewritten to leave behind. Keep the old offset.
	if (device.tareGood < TARE_MIN_GOOD) { device.tareValid = false; return; }

	// Well-formed samples that disagree with each other: EMI corrupting a bit on
	// DOUT, a chip still settling, or the cell genuinely moving. read() validates
	// the clock train and never the data, so this is the only place any of that
	// can be caught.
	if (device.tareSpread > TARE_MAX_SPREAD) { device.tareValid = false; return; }

	device.offset    = (float)(device.tareSum / device.tareGood);
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


