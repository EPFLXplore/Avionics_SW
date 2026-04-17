/*
 * LedsTask.cpp
 *
 *  Created on: Apr 8, 2026
 *      Author: pedro
 */

#include "LedsThread.h"

LedsThread::LedsThread(const char *name, osPriority priority,
		UART_HandleTypeDef *huart_x, TIM_HandleTypeDef *tim_x,
		uint32_t tim_x_ch, bool assignedJob) : MessageThread(name, priority) {
	strip = new LEDStrip(NUM_LEDS);
	job = assignedJob;
	thread_uart = huart_x;
	thread_tim = tim_x;
	thread_tim_ch = tim_x_ch;
}

LedsThread::~LedsThread(){
	if (strip){
		delete strip;
		strip = nullptr;
	}
}


void LedsThread::init(){
	strip->begin(thread_tim, thread_tim_ch);
	strip->clear();

	cmdQueue = osMessageQueueNew(QUEUE_DEPTH, sizeof(Command), NULL);
}

void LedsThread::loop(){
	if (job) serialTask();
	else ledTask();

//	while (popCommand(req)) {
//
//	}
//
//	cmd.system = 1;
//	cmd.mode = 2;
//	cmd.emergency_global = 1;
//	cmd.emergency_motors = 0;
//
//	switch (cmd.system) {
//		case 0: cmd.segment.r = 147; cmd.segment.g = 0;   cmd.segment.b = 211; cmd.segment.low = 0, cmd.segment.high= 50; break; // NAV - Pink
//		case 1: cmd.segment.r = 255; cmd.segment.g = 140; cmd.segment.b = 0;   cmd.segment.low = 51, cmd.segment.high=100; break; // HD - Yellow
//		case 2: cmd.segment.r = 0;   cmd.segment.g = 255; cmd.segment.b = 0;   cmd.segment.low = 0, cmd.segment.high=50; break; // DRILL - Green
//		case 3: cmd.segment.r = 20; cmd.segment.g = 56; cmd.segment.b = 50; cmd.segment.low = 51, cmd.segment.high=100; break; // Avionics - Turquoise
//	}
//
//	if(cmd.mode == 4){
//		cmd.segment.r = 100; cmd.segment.g = 81; cmd.segment.b = 50; cmd.segment.low = 0; cmd.segment.high = 50; // AMBER
//	}
//
//		  //emergency shutdown
//	if(cmd.mode == 5){
//		cmd.segment.r = 255; cmd.segment.g = 0;   cmd.segment.b = 0; cmd.segment.low = 0, cmd.segment.high= 100;
//	}
//
//	strip->applyCommand(cmd);
//	osDelay(1);
//	strip->setBrightness(1);
//	osDelay(1);
//	strip->tick();
//	//strip->tickOneSystem(cmd.system);
//	osDelay(1);
}

void LedsThread::ledTask() {

	cmd.mode = 3;
	cmd.system = 1;

    switch (cmd.system) {
    	case 0: cmd.segment.r = 147; cmd.segment.g = 0;   cmd.segment.b = 211; cmd.segment.low = 0, cmd.segment.high = 50; break; // NAV - Pink
      	case 1: cmd.segment.r = 255; cmd.segment.g = 140; cmd.segment.b = 0;   cmd.segment.low = 51, cmd.segment.high = 100; break; // HD - Yellow
      	case 2: cmd.segment.r = 0;   cmd.segment.g = 255; cmd.segment.b = 0;   cmd.segment.low = 0, cmd.segment.high = 50; break; // DRILL - Green
      	case 3: cmd.segment.r = 20; cmd.segment.g = 56; cmd.segment.b = 50; cmd.segment.low = 51, cmd.segment.high = 100; break; // Avionics - Turquoise
    }

    if(cmd.mode == 4){
        cmd.segment.r = 100; cmd.segment.g = 81; cmd.segment.b = 50; cmd.segment.low = 0; cmd.segment.high = 50; // AMBER
    }

    //emergency shutdown
    if(cmd.mode == 5){
        cmd.segment.r = 255; cmd.segment.g = 0;   cmd.segment.b = 0; cmd.segment.low = 0, cmd.segment.high= 100;
    }

    strip->applyCommand(cmd);

//	if (osMessageQueueGet(cmdQueue, &cmd, NULL, pdMS_TO_TICKS(1)) == osOK) {
//		strip->applyCommand(cmd);
//	}

    //strip->tick();
	strip->tickOneSystem(cmd.system);
	osDelay(1);
}

void LedsThread::serialTask() {
	if (HAL_UART_Receive(thread_uart, &rxByte, 1, 100) == HAL_OK)
	{
		cmdBuff[bufIdx] = rxByte;
		HAL_UART_Transmit(thread_uart, &rxByte, 1, 50);

		if (cmdBuff[bufIdx] == '\r' || bufIdx >= CMD_BUFF_SIZE - 1)
		{
			cmdBuff[bufIdx] = '\0';
			bufIdx = 0;

			uint8_t newline[] = "\r\n";
			HAL_UART_Transmit(thread_uart, newline, 2, 50);

			int low, high, system, mode;

			if (sscanf(cmdBuff, "%d %d %d %d", &low, &high, &system, &mode)
					== 4) {
				cmd.system = (uint8_t) min(max(system, 0), 3);
				cmd.mode = (uint8_t) min(max(mode, 0), 6);


                switch (cmd.system) {
                	case 0: cmd.segment.r = 147; cmd.segment.g = 0;   cmd.segment.b = 211; cmd.segment.low = 0, cmd.segment.high = 50; break; // NAV - Pink
                  	case 1: cmd.segment.r = 255; cmd.segment.g = 140; cmd.segment.b = 0;   cmd.segment.low = 51, cmd.segment.high = 100; break; // HD - Yellow
                  	case 2: cmd.segment.r = 0;   cmd.segment.g = 255; cmd.segment.b = 0;   cmd.segment.low = 0, cmd.segment.high = 50; break; // DRILL - Green
                  	case 3: cmd.segment.r = 20; cmd.segment.g = 56; cmd.segment.b = 50; cmd.segment.low = 51, cmd.segment.high = 100; break; // Avionics - Turquoise
                }
				char confirm[64];
				sprintf(confirm, "Got: sys=%d mode=%d low=%d high=%d\r\n",
						cmd.system, cmd.mode, cmd.segment.low,
						cmd.segment.high);
				HAL_UART_Transmit(thread_uart, (uint8_t*) confirm, strlen(confirm), 100);

				osMessageQueuePut(cmdQueue, &cmd, 0, 0);
			}

			for (size_t i(0); i < CMD_BUFF_SIZE; i++)
				cmdBuff[i] = 0;
		} else
			bufIdx++;
	}

	osDelay(10);
}




