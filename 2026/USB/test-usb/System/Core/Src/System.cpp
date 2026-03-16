/*
 * System->cpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */


#include "System.h"
#include "queue.h"



TestTask* System::test = nullptr;
MicroRosThread* System::microros = nullptr;
HeartBeat* System::beat = nullptr;
ThreadsRegistry System::reg{};

void System::init(){

	//Allocate memory for the test thread
	beat = new HeartBeat();
	test = new TestTask();
	AnalogTask* analog = new AnalogTask();

    reg.beat = beat;
    reg.test = test;
    reg.analog = analog; // Make sure reg has this member!

    microros = new MicroRosThread(&reg);

	test->start();
	beat->start();
	analog->start(); // Start it
	microros->start();

}
//salut
