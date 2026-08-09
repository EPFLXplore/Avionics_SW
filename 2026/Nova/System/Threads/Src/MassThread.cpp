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
	// cell[] own their HX711 (constructed in the header) - nothing to wire.
	// The board profile is NOT read here: see init().
}

MassThread::~MassThread(){
	// cell[] are statically allocated members: nothing to free.
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
	// off cell[i].global_id, so swapping this for hardware detection later
	// changes nothing beyond this one line.
	bindDevices(cell, profile().mass);

	for (MassType& c : cell) c.hx.begin();

	// Wiring probe (debugger-visible verdict per connector, see HX711::lineTest).
	for (MassType& c : cell) c.lineTest = c.hx.lineTest();

	// The probe power-cycles the chips: first conversion lands ~400 ms after
	// wake (10 SPS settling), so give them time before taring.
	osDelay(600);
	for (MassType& c : cell) this->tareScale(c);
}

void MassThread::loop(){
    // Always pop, so the queue drains even for devices this board does not have.
    MassRequest cmd;
    if (this->popCommand(cmd)) {
    	if (MassType* dev = deviceFor(cell, cmd.id)) {
    		if (cmd.change_scale) {
    			dev->slope = cmd.scale; // runtime calibration override
    		}
    		if (cmd.tare) {
    			this->tareScale(*dev);
    		}
    	}
    }

    // Publish only bound cells, so a global id means one physical scale fleet-wide.
    for (MassType& c : cell)
    	if (c.global_id != NO_DEVICE) this->updateMass(c);
}

void MassThread::updateMass(MassType& device){
	this->update(device);
	MassPacket st;
	st.mass = device.weight;
	st.id = device.global_id;
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
	    case HX711::ReadResult::Timeout:    device.nTimeout++;    return;
	    case HX711::ReadResult::ClockFault: device.nClockFault++; return;
	    case HX711::ReadResult::Ok:         break;
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
	// Average 20 fresh samples. read() blocks (bounded) until each one is
	// ready, so no available() pre-check: the old guard silently SKIPPED the
	// tare whenever the chip wasn't ready yet (it rarely is 110 ms after
	// power-up), leaving offset = 0.
	int64_t sum = 0;
	uint8_t good = 0;
	for (uint8_t i = 0; i < 20; ++i) {
		int32_t raw = 0;
		HX711::ReadResult res = device.hx.read(raw);
		if (res == HX711::ReadResult::Ok)      { sum += raw; ++good; }
		else if (res == HX711::ReadResult::Timeout) break; // sensor absent: stop waiting
	}
	if (good == 0) return; // no data at all: keep the previous offset

    device.offset = (float)(sum / good);

    for (uint8_t i = 0; i < AVG_SIZE; ++i) {
    	device.buffer[i] = device.offset;
    }
}

/*
void MassThread::sendfloat(float value) {
	snprintf(this->buffer, sizeof(buffer), "Mass value = %.3f\r\n", value);
	CDC_Transmit_FS((uint8_t*)buffer, sizeof(buffer));
}

void MassThread::sendint(int32_t value) {
	snprintf(this->buffer, sizeof(buffer), "Mass value = %d\r\n", value);
	CDC_Transmit_FS((uint8_t*)buffer, sizeof(buffer));
}
*/


