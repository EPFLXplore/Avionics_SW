/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#ifndef THREADS_INC_TESTTASK_H_
#define THREADS_INC_TESTTASK_H_

#include "MessageThread.h"


class TestTask : public MessageThread<TestPacket,TestPacket> {
public:
	TestTask(const char* name, osPriority priority);

	void init();
	void loop();

private:
	bool cleared = false;
	TestPacket status{};
	TestPacket cmd{};
	uint32_t counter = 0;
};

#endif /* THREADS_INC_TESTTASK_H_ */
