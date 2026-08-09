/*
 * System.hpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */

#pragma once
#include <LedsThread.h>
#include <ServoThread.h>
#include "MassThread.h"
#include "pHMeterThread.h"
#include "SerialThread.h"
#include "HeartBeatThread.h"

class System {
public:
	static void init();

	// The threads, as objects constructed on first use (from init(), i.e. AFTER
	// HAL + the kernel are up). Every driver they own is therefore built post-HAL.
	// Access them as objects: System::servo().pushCommand(...).
	static SerialThread& comms();
	static ServoThread&  servo();
	static MassThread&   mass();
	static pHMeterThread& ph();
	static LedsThread&   leds();
	static HeartBeatThread&    heartbeat();
};

