/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#ifndef THREADS_INC_HEARTBEAT_H_
#define THREADS_INC_HEARTBEAT_H_

#include "Thread.h"

class HeartBeat : public Thread {
public:
	HeartBeat(QueueHandle_t toRosQueue);

	void init();
	void loop();

private:
	uint32_t counter = 0;
	QueueHandle_t queue_to_ros;
};

#endif /* THREADS_INC_TESTTASK_H_ */
