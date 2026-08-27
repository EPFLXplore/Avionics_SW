/*
 * Thread.cpp
 *
 *  Created on: 2021
 *      Author: AV Team 2021
 */

#include <stdarg.h>
#include <string.h>
#include <Thread.h>


/** Danger zone: changing the stack size might create very nasty bugs. Capped by
 *  Thread::start() to the per-thread static buffer (STACK_BYTES). */
/* 4096, matching Thread::STACK_BYTES - the per-task buffer is statically sized
 * at 4 KB whatever we ask for, so anything less just leaves part of it
 * unreachable. It is NOT free to get this wrong: SerialThread derives from
 * Thread directly (not MessageThread), so it takes THIS default, and its
 * SerialProtocol::poll() frame is 1240 bytes once RX_CHUNK is a full USB-FS
 * frame. At 2048 that plus the TX path and the interrupt frame ran the task
 * stack out, and a static FreeRTOS task with no overflow checking simply
 * corrupts whatever sits next to it. */
inline constexpr uint32_t DEFAULT_STACK_SIZE = 4096;


void taskRun(void* arg) {
	Thread* thread = (Thread*) arg;

	osDelay(pdMS_TO_TICKS(thread->getDelay()));

	thread->init();

	while(thread->isRunning()) {
		thread->loop();
		osDelay(pdMS_TO_TICKS(thread->getDelay()));
		//taskYIELD();
	}

	// Thread objects are statically allocated (see System.cpp) - never deleted.
	vTaskDelete(nullptr);

	while(true) {
		osDelay(pdMS_TO_TICKS(1000));
	}
}

Thread::Thread(const char* name) : Thread(name, (osPriority) osPriorityNormal) {
	;
}

Thread::Thread(const char* name, osPriority priority) : Thread(name, priority, DEFAULT_STACK_SIZE) {
	;
}

Thread::Thread(const char* name, uint32_t stackSize) : Thread(name, (osPriority) osPriorityNormal, stackSize) {
	;
}

Thread::Thread(const char* name, osPriority priority, uint32_t stackSize){
	this->_attributes.name = (char*) name;
	this->_attributes.stack_size = stackSize;
	this->_attributes.priority = (osPriority_t) priority;
}

void Thread::start() {
    if (_started) return;
    this->_started = true;

    // Static allocation: hand the task its stack + control block so osThreadNew
    // creates it via xTaskCreateStatic (no pvPortMalloc).
    if (_attributes.stack_size == 0 || _attributes.stack_size > sizeof(_stackBuffer)) {
        _attributes.stack_size = sizeof(_stackBuffer);
    }
    _attributes.stack_mem = _stackBuffer;
    _attributes.cb_mem    = &_tcbBuffer;
    _attributes.cb_size   = sizeof(_tcbBuffer);

    this->_handle = osThreadNew(taskRun, this, &_attributes);
    configASSERT(_handle);
    this->_name = _attributes.name;
}

osThreadId_t Thread::getHandle() {
	return _handle;
}

const char* Thread::getName(){
	return this->_name;
}

void Thread::terminate() {
	this->_running = false;
}


void Thread::setDelay(uint32_t ms) {
	this->_delay = ms;
}

uint32_t Thread::getDelay() {
	return this->_delay;
}
