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


class System {
public:
	static void init();

	static TestTask* test;
	static MassThread* mass;
	static MicroRosThread* microros;
	static QueueHandle_t queue_to_ros;
	static HeartBeat* beat;
};

#endif /* CORE_INC_SYSTEM_H_ */
