/*
 * TestTask.cpp
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#include <HeartBeat.h>
#include "System.h"
#include "usbd_cdc_if.h"


// syscalls_time.c
#include <sys/time.h>
#include "cmsis_os2.h"


HeartBeat::HeartBeat(QueueHandle_t toRosQueue) : Thread("HeartBeat", (osPriority)osPriorityNormal5, (uint32_t) 2048), queue_to_ros(toRosQueue) {}

void HeartBeat::init(){

}

void HeartBeat::loop(){
	if (queue_to_ros == nullptr)
	        return;

	    SystemMessage msg;
	    msg.type = PacketType::HEARTBEAT;

	    msg.data.heartbeat.beat = counter++;

	    // non-blocking send
	    xQueueSend(queue_to_ros, &msg, 0);
}






