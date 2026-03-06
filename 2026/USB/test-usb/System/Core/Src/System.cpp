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
MassThread* System::mass = nullptr;
ServoThread* System::servo = nullptr;
ThreadsRegistry System::reg{};

void System::init(){

	//Allocate memory for the test thread
	beat = new HeartBeat();
	test = new TestTask();
	mass = new MassThread();
	servo = new ServoThread();

    reg.beat = beat;
    reg.test = test;
    reg.mass = mass;
    reg.servo = servo;

    //servo->init();
    microros = new MicroRosThread(&reg);

	test->start();
	beat->start();
	mass->start();
	microros->start();
	servo->init();
	servo->start();

}

