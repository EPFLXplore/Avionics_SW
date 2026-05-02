/*
 * LedsTask.cpp
 *
 *  Created on: Apr 8, 2026
 *      Author: pedro
 */

#include <LedsThread.h>
#include "stm32g4xx.h"

#include "Timers.h"

#define NUM_LEDS 34
#define DMA_BUFF_SIZE NUM_LEDS*BITS_PER_LED + RESET_PULSE


LedsThread::LedsThread(const char* name, osPriority priority) : MessageThread(name, priority) {
	strip = new LEDStrip(NUM_LEDS);
}

LedsThread::~LedsThread(){
	if (strip){
		delete strip;
		strip = nullptr;
	}
}


void LedsThread::init(){
	  strip->begin(&htim15, TIM_CHANNEL_1);
//	  cmd.segment.r = 0;
//	  cmd.segment.g = 255;
//	  cmd.segment.b = 0;
//	  cmd.segment.low = 0;
//	  cmd.segment.high = 50;
//
//	  strip->applyCommand(cmd);
//	  osDelay(1);
//	  strip->setBrightness(70);

//	  strip->tick();

}

void LedsThread::loop(){
//	cmd.system = 0;
//	cmd.mode = 0;
//	cmd.emergency_global = 1;
//	cmd.emergency_motors = 0;
//
//	switch (cmd.system) {
//	case 0:
//		cmd.segment.r = 147;
//		cmd.segment.g = 0;
//		cmd.segment.b = 211;
//		cmd.segment.low = 0, cmd.segment.high = 50;
//		break; // NAV - Pink
//	case 1:
//		cmd.segment.r = 255;
//		cmd.segment.g = 1401000;
//		cmd.segment.b = 0;
//		cmd.segment.low = 51, cmd.segment.high = 100;
//		break; // HD - Yellow
//	case 2:
//		cmd.segment.r = 0;
//		cmd.segment.g = 255;
//		cmd.segment.b = 0;
//		cmd.segment.low = 0, cmd.segment.high = 50;
//		break; // DRILL - Green
//	case 3:
//		cmd.segment.r = 20;
//		cmd.segment.g = 56;
//		cmd.segment.b = 50;
//		cmd.segment.low = 51, cmd.segment.high = 100;
//		break; // Avionics - Turquoise
//	}
//
//	if (cmd.mode == 4) {
//		cmd.segment.r = 100;
//		cmd.segment.g = 81;
//		cmd.segment.b = 50;
//		cmd.segment.low = 0;
//		cmd.segment.high = 50; // AMBER
//	}
//
//	//emergency shutdown
//	if (cmd.mode == 5) {
//		cmd.segment.r = 255;
//		cmd.segment.g = 0;
//		cmd.segment.b = 0;
//		cmd.segment.low = 0, cmd.segment.high = 100;
//	}
//
//	strip->applyCommand(cmd);
//	osDelay(1);
//	strip->tickOneSystem(cmd.system);
//	osDelay(1);

	while (popCommand(req)) {

//		strip->clear();

		cmd.system = req.system;
		cmd.mode = req.mode;
		cmd.emergency_global = 1;
		cmd.emergency_motors = 0;

		switch (cmd.system) {
			case 0: cmd.segment.r = 147; cmd.segment.g = 0;   cmd.segment.b = 211; cmd.segment.low = 0, cmd.segment.high= 50; break; // NAV - Pink
			case 1: cmd.segment.r = 255; cmd.segment.g = 140; cmd.segment.b = 0;   cmd.segment.low = 51, cmd.segment.high=100; break; // HD - Yellow
			case 2: cmd.segment.r = 0;   cmd.segment.g = 255; cmd.segment.b = 0;   cmd.segment.low = 0, cmd.segment.high=50; break; // DRILL - Green
			case 3: cmd.segment.r = 20; cmd.segment.g = 56; cmd.segment.b = 50; cmd.segment.low = 51, cmd.segment.high=100; break; // Avionics - Turquoise
		}

		if(cmd.mode == 4){
			cmd.segment.r = 100; cmd.segment.g = 81; cmd.segment.b = 50; cmd.segment.low = 0; cmd.segment.high = 50; // AMBER
		}

			  //emergency shutdown
		if(cmd.mode == 5){
			cmd.segment.r = 255; cmd.segment.g = 0;   cmd.segment.b = 0; cmd.segment.low = 0, cmd.segment.high= 100;
		}

//		vTaskSuspendAll();
		strip->applyCommand(cmd);

//		xTaskResumeAll();
	}

	strip->tickOneSystem(cmd.system);
}






