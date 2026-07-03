/*
 * System.hpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */

#ifndef CORE_INC_SYSTEM_H_
#define CORE_INC_SYSTEM_H_

#include <LedsThread.h>
#include <ServoThread.h>
#include "MassThread.h"
#include "SerialThread.h"
#include "HeartBeat.h"

class System {
public:
	static void init();

	// The threads, as objects constructed on first use (from init(), i.e. AFTER
	// HAL + the kernel are up). Every driver they own is therefore built post-HAL.
	// Access them as objects: System::servo().pushCommand(...).
	static SerialThread& comms();
	static ServoThread&  servo();
	static MassThread&   mass();
	static LedsThread&   leds();
	static HeartBeat&    heartbeat();
};

#endif /* CORE_INC_SYSTEM_H_ */
