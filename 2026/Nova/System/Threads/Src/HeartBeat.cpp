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

void HeartBeat::loop()
{
    // Liveness beat: one status per loop; the wire owner (SerialThread) drains
    // it onto the link. The counter lets the receiver spot dropped beats.
    // Loop period is controlled by Thread::setDelay().
    Heartbeat st{};
    st.dummy = ++_count;
    pushStatus(st);
}






