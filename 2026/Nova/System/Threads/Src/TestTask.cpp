/*
 * TestTask.cpp
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#include <TestTask.h>
#include "System.h"
#include "usbd_cdc_if.h"


// syscalls_time.c
#include <sys/time.h>
#include "cmsis_os2.h"


TestTask::TestTask(const char* name, osPriority priority) : MessageThread(name, priority) {}

void TestTask::init(){
	counter = 0;
}

void TestTask::loop(){
	while (popCommand(cmd)) {
	    // Add received ping to our counter
		counter += cmd.ping;
	}


	status.ping = counter;   // or use a different field if you have one
	pushStatus(status);
}






