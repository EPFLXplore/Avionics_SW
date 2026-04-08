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
    // If you ever add commands, handle them here:
    // EmptyMessage cmd;
    // while (popCommand(cmd)) {
    //     // no-op for now
    // }

    // 1) Update your heartbeat value
    // For now let's just increment a counter as a test signal.
    // Later you can set this from a real sensor / timer.
	_beat += 1.0f;

    // 2) Push status to MicroRosThread
    BeatPacket st;
    st.beat = _beat;
    pushStatus(st);

    // loop period is controlled by Thread::setTickDelay()
}






