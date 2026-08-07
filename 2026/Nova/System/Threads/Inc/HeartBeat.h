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
    void setID(uint8_t id);

private:
    uint8_t _board_id = 0;   // this master's 2-bit strap id, set via setID()
};


#endif /* THREADS_INC_TESTTASK_H_ */
