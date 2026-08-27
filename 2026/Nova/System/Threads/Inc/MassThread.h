/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#pragma once

#include "Thread.h"
#include "MessageThread.h"
#include "HX711.h"
#include "packets.h"
#include "device_ids.h"    // MassId: the shared, fleet-wide device ids
#include "BoardProfile.h"  // which connector carries which load _cell, per board
#include "Pins.h"          // pinOf(): that connector's pads

#include <sys/time.h>
#include "cmsis_os2.h"
#include "usbd_cdc_if.h"

#include <stdio.h>

// Sized for RATE=1 (80 SPS, SJ2 on the hat): 160 samples = a 2 s window, the
// same wall-clock averaging as 20 samples at 10 SPS, but with 8x the samples in
// it. Table 2 gives 90 nV input noise at 80 SPS against 50 nV at 10, so the net
// is 90/sqrt(160) vs 50/sqrt(20) = 1.57x QUIETER, and Nyquist moves 5 Hz -> 40 Hz
// so rover vibration in the 5-40 Hz band is sampled instead of aliased to DC.
//
// Going back to 10 SPS means AVG_SIZE 20 AND mass().setDelay(100) in System.cpp.
// Both, or the buffer spans the wrong amount of time.
//
// Ceiling: this is uint8_t and movingAverage() takes a uint8_t count - 255 is the max.
constexpr uint8_t AVG_SIZE = 160;

// The thread now polls at 100 Hz to keep up with the chip, but the wire does not
// need that: updateMass() would otherwise pushStatus() 100x/s per cell, ten
// times the old rate, on a link whose 3 s stall detector re-opens the port and
// replays calibration when it trips. Sample fast, publish at the old ~10 Hz.
// The 2 s moving average makes anything faster meaningless anyway, and
// calibrate_mass.sh's NSAMPLES=8 window assumes ~10 Hz.
constexpr uint8_t PUBLISH_EVERY = 10;   // loops per published packet

// --- tare acceptance --------------------------------------------------------
// The tare builds the zero every reading is measured against, and a wrong
// zero is a CONSTANT error in grams at every load - at a single reference mass
// that is indistinguishable from a wrong slope. These are the gates that stop a
// bad zero from being adopted silently.
// 80 samples at 80 SPS = 1 s, HALF the old tare's wall time and slightly quieter
// than it: 90/sqrt(80) = 10.1 nV against the old 50/sqrt(20) = 11.2 nV. Keeping
// 20 here would have made the zero NOISIER at 80 SPS, since the per-sample noise
// is 1.8x higher - the sample count has to follow the rate.
constexpr uint8_t TARE_SAMPLES      = 80;  // attempts per tare (~1 s at 80 SPS)
constexpr uint8_t TARE_MIN_GOOD     = 48;  // 60% floor, as before: below this the
                                           // sqrt(N) the average promises is
                                           // largely gone, and at 1 the zero is
                                           // ONE sample
constexpr uint8_t TARE_MAX_TIMEOUTS = 10;   // budget before abandoning the tare

// Rejection threshold for the spread (max-min) of the tare samples, in raw
// counts. MEASURE THIS: watch lastRaw with the pan empty and the robot in its
// working state, and take the observed peak-to-peak. The value below is 2% of a
// 1 kg reading (~20 g), mirroring the steadiness gate calibrate_mass.sh already
// applies to the LOADED reading - a starting point, not a measurement.
//
// At 80 SPS expect it to need RAISING: per-sample noise is 1.8x higher and the
// tare now takes 80 samples instead of 20, and peak-to-peak grows with both. Set
// it too tight and every tare is refused.
constexpr int32_t TARE_MAX_SPREAD = 38878;

struct MassType {
	// Global device id on this connector, bound in MassThread::init() from the
	// board profile, or NO_DEVICE when nothing is plugged in here. This is what
	// the wire carries: a _cell publishes under it and answers commands for it.
	uint8_t globalId = NO_DEVICE;
	HX711 hx;                          // each _cell owns its sensor (no pointers)

	/* FALLBACK ONLY: mass_cal.yaml on the RPi is the
	    * source of truth, replayed over MassRequest on every link-up.
	    * WHY? because its more practical for last minute calibrations
	    *
	    * RECOMENDATION: move from yaml to a plain old .h on the new submodule so to synch up both
	    * you only need to rebuild and reflash but you still get the practicality of changing calibration from the RP side.
	    *
	    * */
	float offset = 0.0f;
	float slope = 0.0005157476f;


	/* why having here an offset and a slope apaprt from the driver one? Because this makes the driver portable to other systems that dont have RTOS */
	float weight = 0.0f;
	float buffer[AVG_SIZE] = {};
	// Live diagnostics. Members of the static MassThread (fixed addresses), so
	// the debugger can watch them while running - unlike the stack local in
	// update(), which reads stale when available() bails before the read.
	int32_t  lastRaw     = 0;   // last GOOD sample
	uint32_t nGood       = 0;   // successful reads
	uint32_t nNotReady   = 0;   // DOUT high at poll: no conversion (power / DOUT line)
	uint32_t nClockFault = 0;   // chip ignored our clocks (SCK line open)
	uint32_t nTimeout    = 0;   // ready flag lost while waiting
	uint8_t  tareGood    = 0;     // samples that survived the last tare
	int32_t  tareSpread  = 0;     // max-min across those samples, raw counts
	bool     tareValid   = false; // false until a tare passes both gates
	uint8_t  pubTick     = 0;     // loops since the last published packet



	// --- tare in progress -------------------------------------------------
	// The tare used to run to completion inside one loop() call: TARE_SAMPLES
	// blocking reads back to back, ~1 s at 80 SPS, during which this thread
	// serviced no commands and published no packets. A tare arriving mid-burst
	// therefore froze the consumer for a second while its queue kept filling.
	// It is now one sample per loop, accumulated here across ~80 loops, so the
	// thread stays responsive throughout and only THIS cell pauses.
	// `taring` is the whole state: false = idle, and every field below is
	// scratch that beginTare() resets.
	bool     taring       = false;
	uint8_t  tareTries    = 0;    // samples attempted so far (good + faulted)
	uint8_t  tareTimeouts = 0;    // timeouts so far, against TARE_MAX_TIMEOUTS
	int64_t  tareSum      = 0;    // running sum of the good samples
	int32_t  tareLo       = 0;    // running min / max, for the spread gate
	int32_t  tareHi       = 0;
	explicit MassType(const HX711& sensor) : hx(sensor) {}
};


class MassThread : public MessageThread<MassRequest, MassPacket>{
public:
	MassThread(const char* name, osPriority priority);
	~MassThread();

	/** True when this board carries at least one load _cell. Read live off the
	 *  profile at the moment System asks, NOT latched at construction: that
	 *  keeps the start decision where HEAD had it - in System::init, at start
	 *  time - instead of freezing it into a bool several statements earlier.
	 *  System asks this instead of reading the profile itself, so nothing above
	 *  the threads knows what a connector is. */
	bool hasDevices() const { return anySlot(DeviceType::LoadCell); }

	/** Debugger handle: the _cell on a given connector, whatever is bound to it. */
	MassType& connector(ConnType s) { return _cell[idOf(s) - CONNECTOR_FIRST]; }

	void init();

	void updateMass(MassType& device);

	void shift(float *array , int N, float valueIn);

	float movingAverage(const float *samples, uint8_t count);

	void update(MassType& device);

	/** Arm a tare on this cell. Returns immediately: the sampling happens one
	 *  reading per loop() in stepTare(). Arming a cell that is already taring
	 *  restarts it from zero, which is what a second request means. */
	void beginTare(MassType& device);

	/** Take ONE sample into the running tare, and commit when the sample budget
	 *  or the timeout budget runs out. Only call while device.taring. */
	void stepTare(MassType& device);

	/** Apply the two acceptance gates and adopt (or refuse) the new zero.
	 *  Clears device.taring either way. */
	void commitTare(MassType& device);

	void loop();

private:
	// One entry per connector slot, in ConnType order (HX711's ctor only stores
	// GPIO refs, no HAL, so constructing them here is safe). Adding a connector
	// is one more line here plus one more id per profile row - the loops below
	// and every helper are length-generic.
	MassType _cell[CONNECTOR_COUNT] = {
		MassType{HX711{pinOf(ConnType::ConnI2C)}},
		MassType{HX711{pinOf(ConnType::ConnUART)}},
	};

	char _buffer[64];

	//Tiny helper for now DONT USE WITH MICROROS
	void sendfloat(float value);
	void sendint(int32_t value);
};


