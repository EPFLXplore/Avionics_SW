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


TestTask::TestTask() : Thread("TestingTask", (osPriority)osPriorityNormal5, (uint32_t) 2048) {}

void TestTask::init(){

}

void TestTask::loop(){
	//CDC_Transmit_FS((uint8_t*) "Hello my friend", 15);
}






