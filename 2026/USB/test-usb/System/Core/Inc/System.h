/*
 * System.hpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */

#ifndef CORE_INC_SYSTEM_H_
#define CORE_INC_SYSTEM_H_

#include <MicroRosThread.h>
#include "TestTask.h"
#include "MassThread.h"
#include "HeartBeat.h"
#include "AnalogTask.h"


struct ThreadsRegistry {
    HeartBeat* beat;
    TestTask* test;     // or whatever TestTask really is
    // Add more threads here as you create them
    AnalogTask* analog;

};

class System {
public:
	static void init();

	static TestTask* test;
	static MicroRosThread* microros;
	static HeartBeat* beat;
	static AnalogTask* analog;
	static ThreadsRegistry reg;
};

#endif /* CORE_INC_SYSTEM_H_ */
