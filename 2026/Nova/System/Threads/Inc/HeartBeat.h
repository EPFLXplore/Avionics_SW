/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#ifndef THREADS_INC_HEARTBEAT_H_
#define THREADS_INC_HEARTBEAT_H_

#include "MessageThread.h"

class HeartBeat : public MessageThread<EmptyMessage, Heartbeat> {
public:
    HeartBeat(const char* name, osPriority priority);
    void init() override;
    void loop() override;

private:
    uint8_t _count = 0;   // free-running beat counter (wraps at 256)
};


#endif /* THREADS_INC_TESTTASK_H_ */
