/*
 * System->cpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */


#include "System.h"
#include "queue.h"



TestTask* System::test = nullptr;
MassThread* System::mass = nullptr;
MicroRosThread* System::microros = nullptr;
QueueHandle_t System::queue_to_ros = nullptr; //QueueHandle_t is actually a typedef of a pointer
HeartBeat* System::beat = nullptr;

void System::init(){

	queue_to_ros = xQueueCreate(32, sizeof(SystemMessage));
	configASSERT(queue_to_ros != nullptr);

	//Allocate memory for the test thread
	beat = new HeartBeat(queue_to_ros);
	mass = new MassThread(queue_to_ros);
	microros = new MicroRosThread(queue_to_ros);

	test->start();
	beat->start();
	mass->start();
	microros->start();

}
//salut
