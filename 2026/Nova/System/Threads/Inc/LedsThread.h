/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#ifndef THREADS_INC_LEDSTHREAD_H_
#define THREADS_INC_LEDSTHREAD_H_

#include "MessageThread.h"
#include "LEDStrip.h"

class LedsThread : public MessageThread<LedRequest, EmptyMessage> {
public:
	LedsThread(const char* name, osPriority priority);
	~LedsThread();

	void init();
	void loop();

private:
	bool cleared = false;
	LedRequest req{};
	LEDStrip* strip1;
	Command cmd;
};

#endif /* THREADS_INC_LEDSTHREAD_H_ */
