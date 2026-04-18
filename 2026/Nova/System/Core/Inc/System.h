/*
 * System.hpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */

#ifndef CORE_INC_SYSTEM_H_
#define CORE_INC_SYSTEM_H_

#include <LedsThread.h>
#include <MicroRosThread.h>
#include "MassThread.h"
#include "HeartBeat.h"
#include "servoThreadnew.h"



struct ThreadsRegistry {

    HeartBeat* beat;
    MassThread* mass;
    ServoThread* servo;
    LedsThread* leds;
    // Add more threads here as you create them
};

class System {
public:
	static void init();

	static LedsThread* leds;
	//static MicroRosThread* microros;
	static HeartBeat* beat;
	static ThreadsRegistry reg;
	static MassThread* mass;
	static ServoThread* servo;
};

#endif /* CORE_INC_SYSTEM_H_ */
