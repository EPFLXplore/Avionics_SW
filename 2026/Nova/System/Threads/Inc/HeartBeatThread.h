/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#pragma once
#include "MessageThread.h"

class HeartBeatThread : public MessageThread<EmptyMessage, Heartbeat> {
public:
    HeartBeatThread(const char* name, osPriority priority);
    void init() override;
    void loop() override;
    void setID(uint8_t id);

private:
    uint8_t _boardId = 0;   // this master's 2-bit strap id, set via setID()
};


