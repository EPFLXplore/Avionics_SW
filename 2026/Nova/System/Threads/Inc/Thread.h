/*
 * Thread.hpp
 *
 *  Created on: 23 Oct 2020
 *      Author: AV Team 2020
 */

#ifndef THREAD_H_
#define THREAD_H_

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "semphr.h"

class Thread {
public:
	Thread(const char* name);
	Thread(const char* name, osPriority priority);
	Thread(const char* name, uint32_t stackSize);
	Thread(const char* name, osPriority priority, uint32_t stackSize);
	virtual ~Thread() {};
	virtual void init() = 0;
	virtual void loop() = 0;

	void start();

	osThreadId getHandle();
	bool isRunning() { return running; }
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

	osThreadId_t   handle{nullptr};
	osThreadAttr_t attributes{};                       // saved until start()
	const char*    name{nullptr};
	uint32_t       delay{100};
	bool           running{true};
	bool           started{false};

	StackType_t    stackBuffer_[STACK_BYTES / sizeof(StackType_t)]{};
	StaticTask_t   tcbBuffer_{};
};


#endif /* THREAD_H_ */

