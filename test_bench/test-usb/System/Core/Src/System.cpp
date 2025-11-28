/*
 * System->cpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */


#include "System.h"



TestTask* System::test = nullptr;
MassThread* System::mass = nullptr;

void System::init(){
	//Allocate memory for the test thread
	test = new TestTask();
	mass = new MassThread();

	//test->start();
	mass->start();
}
//salut
