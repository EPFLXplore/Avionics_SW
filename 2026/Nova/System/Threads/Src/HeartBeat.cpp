/*
 * TestTask.cpp
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#include "HeartBeat.h"

HeartBeat::HeartBeat(const char* name, osPriority priority)
: MessageThread(name, priority)
{
}

void HeartBeat::init()
{
    // Any hardware init if needed
}

void HeartBeat::setID(uint8_t id){
	_board_id = id;
}

void HeartBeat::loop()
{
    // Liveness beat: one status per loop; the wire owner (SerialThread) drains
    // it onto the link. The payload identifies which master board is alive.
    // Loop period is controlled by Thread::setDelay().
    Heartbeat st{};
    st.board_id = _board_id;
    pushStatus(st);
}






