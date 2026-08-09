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
#include "BoardProfile.h"  // which MassId sits on which connector, per board

#include <sys/time.h>
#include "cmsis_os2.h"
#include "usbd_cdc_if.h"

#include <stdio.h>

constexpr uint8_t AVG_SIZE = 20;

struct MassType {
	// Global device id on this connector, bound in MassThread::init() from the
	// board profile, or NO_DEVICE when nothing is plugged in here. This is what
	// the wire carries: a cell publishes under it and answers commands for it.
	uint8_t global_id = NO_DEVICE;
	HX711 hx;                          // each cell owns its sensor (no pointers)
	float offset = 0.0f;
	float slope = -0.0014164446f; // fallback: overridden at runtime by the slope Nexus replays from mass_cal.yaml
	float weight = 0.0f;
	float buffer[AVG_SIZE] = {};
	// Live diagnostics. Members of the static MassThread (fixed addresses), so
	// the debugger can watch them while running - unlike the stack local in
	// update(), which reads stale when available() bails before the read.
	int32_t  lastRaw     = 0;   // last GOOD sample
	uint8_t  lineTest    = 0xFF; // wiring probe from init(); see HX711::lineTest()
	uint32_t nGood       = 0;   // successful reads
	uint32_t nNotReady   = 0;   // DOUT high at poll: no conversion (power / DOUT line)
	uint32_t nClockFault = 0;   // chip ignored our clocks (SCK line open)
	uint32_t nTimeout    = 0;   // ready flag lost while waiting
	explicit MassType(const HX711& sensor) : hx(sensor) {}
};

// MassConnectors / MASS_CONNECTOR_COUNT live in BoardProfile.h, beside the table
// they index. Only the wire speaks MassId; everything here speaks connectors.

class MassThread : public MessageThread<MassRequest, MassPacket>{ //TODO reput the tare
public:
	MassThread(const char* name, osPriority priority);
	~MassThread();

	/** True when this board carries at least one load cell. Read live off the
	 *  profile at the moment System asks, NOT latched at construction: that
	 *  keeps the start decision where HEAD had it - in System::init, at start
	 *  time - instead of freezing it into a bool several statements earlier.
	 *  System asks this instead of reading the profile itself, so nothing above
	 *  the threads knows what a connector is. */
	bool hasDevices() const { return anyDevice(profile().mass); }

	/** Debugger handle: the cell on a given connector, whatever is bound to it. */
	MassType& connector(MassConnectors c) { return cell[id_of(c)]; }

	void init();

	void updateMass(MassType& device);

	void shift(float *array , int N, float valueIn);

	float movingAverage(const float *arr, uint8_t n);

	void update(MassType& device);

	void tareScale(MassType& device);

	void loop();

private:
	// One entry per MassConnectors slot, in that order (HX711's ctor only stores
	// GPIO refs, no HAL, so constructing them here is safe). Adding a connector
	// is one more line here plus one more id per profile row - the loops below
	// and every helper are length-generic.
	MassType cell[MASS_CONNECTOR_COUNT] = {
		MassType{HX711{HX711_DATA_GPIO_Port, HX711_DATA_Pin, HX711_CLK_GPIO_Port, HX711_CLK_Pin}},
		MassType{HX711{HX2_DATA_GPIO_Port,   HX2_DATA_Pin,   HX2_CLK_GPIO_Port,   HX2_CLK_Pin}},
	};

	char buffer[64];

	//Tiny helper for now DONT USE WITH MICROROS
	void sendfloat(float value);
	void sendint(int32_t value);
};


