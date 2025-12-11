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

    reg.beat = beat;
    reg.test = test;

    microros = new MicroRosThread(&reg);

	test->start();
	beat->start();
	microros->start();

}
//salut
