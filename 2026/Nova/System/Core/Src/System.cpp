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
MicroRosThread* System::microros = nullptr;
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
    microros = new MicroRosThread(&reg, "mROS", osPriorityHigh);

    //Set task timings
    microros->setDelay(10); //ms — fast subscription drain; both servo msgs batch in one tick
    // servo: leave at 0 (set in ctor) so it blocks in waitCommand() and wakes the instant a command queues
    mass->setDelay(100); //ms
    leds->setDelay(300); //ms

	//test->start();
	//beat->start();

	microros->start();
	servo->start();
	mass->start();
	//leds->start();

}

