/*
 * System->cpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */


#include "System.h"
#include "queue.h"
#include "cmsis_os2.h"



LedsThread* System::leds = nullptr;
//MicroRosThread* System::microros = nullptr;
HeartBeat* System::beat = nullptr;
MassThread* System::mass = nullptr;
ServoThread* System::servo = nullptr;
ThreadsRegistry System::reg{};

void System::init(){

	//Allocate memory for the threads
	//beat = new HeartBeat("beat", osPriorityLow);
	//test = new TestTask("test", osPriorityLow);
	mass  = new MassThread("MassThread",  osPriorityNormal);
	servo = new ServoThread("ServoThread", osPriorityNormal);
	leds = new LedsThread("LedsThread", osPriorityNormal);

	//Register tasks
    reg.beat = beat;
    reg.leds = leds;
    reg.mass = mass;
    reg.servo = servo;

    // Allocate memory for the MicroRosThread
    //microros = new MicroRosThread(&reg, "mROS", osPriorityHigh);

    //Set task timings
    //microros->setDelay(700); //ms
    servo->setDelay(100); //ms
    mass->setDelay(100); //ms

	//test->start();
	//beat->start();
	//microros->start();
	servo->start();
	mass->start();
	leds->start();

}

