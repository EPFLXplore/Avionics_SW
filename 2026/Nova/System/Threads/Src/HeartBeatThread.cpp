/*
 * TestTask.cpp
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#include "HeartBeatThread.h"

HeartBeatThread::HeartBeatThread(const char* name, osPriority priority)
: MessageThread(name, priority)
{
}

void HeartBeatThread::init()
{}

void HeartBeatThread::setID(uint8_t id){
	_boardId = id;
}

void HeartBeatThread::loop()
{
    // Health beat: one status per loop; the USB owner (SerialThread) drains
    // it onto the link. The payload identifies which master board is alive.
    Heartbeat st{};
    st.board_id = _boardId;
    pushStatus(st);
}






