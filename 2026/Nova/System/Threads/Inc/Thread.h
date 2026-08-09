/*
 * Thread.hpp
 *
 *  Created on: 23 Oct 2020
 *      Author: AV Team 2020
 */

#pragma once
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "semphr.h"

class Thread {
public:
	Thread(const char* _name);
	Thread(const char* _name, osPriority priority);
	Thread(const char* _name, uint32_t stackSize);
	Thread(const char* _name, osPriority priority, uint32_t stackSize);
	virtual ~Thread() {};
	virtual void init() = 0;
	virtual void loop() = 0;

	void start();

	osThreadId getHandle();
	bool isRunning() { return _running; }
	void terminate();
	void setDelay(uint32_t ms);
	uint32_t getDelay();

	void LOG_INFO(const char* format, ...);
	void LOG_SUCCESS(const char* format, ...);
	void LOG_ERROR(const char* format, ...);

	const char* getName();

private:
	void task();

	// Per-thread static storage so the FreeRTOS task uses NO heap. Sized for the
	// largest stack any thread requests (4 KB); smaller requests use part of it.
	static constexpr uint32_t STACK_BYTES = 4096;

	osThreadId_t   _handle{nullptr};
	osThreadAttr_t _attributes{};                       // saved until start()
	const char*    _name{nullptr};
	uint32_t       _delay{100};
	bool           _running{true};
	bool           _started{false};

	StackType_t    _stackBuffer[STACK_BYTES / sizeof(StackType_t)]{};
	StaticTask_t   _tcbBuffer{};
};


