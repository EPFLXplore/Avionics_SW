/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#ifndef THREADS_INC_HEARTBEAT_H_
#define THREADS_INC_HEARTBEAT_H_

#include "MessageThread.h"

class HeartBeat : public MessageThread<EmptyMessage, BeatPacket> {
public:
    HeartBeat(const char* name, osPriority priority);
    void init() override;
    void loop() override;

private:
    float _beat = 0.0f;   // or int32_t, etc.
};


#endif /* THREADS_INC_TESTTASK_H_ */
