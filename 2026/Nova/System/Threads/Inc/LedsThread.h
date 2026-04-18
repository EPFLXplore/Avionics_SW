/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#ifndef THREADS_INC_LEDSTHREAD_H_
#define THREADS_INC_LEDSTHREAD_H_

#include <bits/stdc++.h>
#include "MessageThread.h"
#include "LEDStrip.h"

#define NUM_LEDS 34
#define DMA_BUFF_SIZE NUM_LEDS*BITS_PER_LED + RESET_PULSE
#define CMD_BUFF_SIZE 64
#define QUEUE_DEPTH 16

class LedsThread : public MessageThread<LedRequest, EmptyMessage> {
public:
	LedsThread(const char* name, osPriority priority, /*UART_HandleTypeDef* huart_x,*/ TIM_HandleTypeDef* tim_x,
			uint32_t tim_x_ch, bool assignedJob);
	~LedsThread();

	void init();
	void loop();

	void ledTask();
	void serialTask();

private:
	bool cleared = false;
	bool job = false; // FALSE: Command reception and LEDStrip control (LedTask) | TRUE: UART Command reception. (SerialTask)

	LedRequest req{};
	osMessageQueueId_t cmdQueue;
	LEDStrip* strip;
	Command cmd;
	char cmdBuff[CMD_BUFF_SIZE];

//	UART_HandleTypeDef* thread_uart;
	TIM_HandleTypeDef* thread_tim;
	uint32_t thread_tim_ch;
	uint8_t rxByte;
	uint8_t bufIdx;
};

#endif /* THREADS_INC_LEDSTHREAD_H_ */
