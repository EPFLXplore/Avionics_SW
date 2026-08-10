/*
 * LedsTask.cpp
 *
 *  Created on: Apr 8, 2026
 *      Author: pedro
 */

#include <LedsThread.h>
#include "stm32g4xx.h"

#include "Pins.h"



LedsThread::LedsThread(const char* name, osPriority priority) : MessageThread(name, priority) {
	// strip points at the static LEDStrip member (no heap).
}

LedsThread::~LedsThread(){
	// LEDStrip is a static member: nothing to free.
}


void LedsThread::init(){
	// Same lookup a servo on this slot would do: a strip and a servo on one slot
	// are the same physical output, so the timer and channel come from one
	// table. The slot itself is LED_STRIP_SLOT, never a literal - move the strip
	// there and here follows, including the driver's DMA request.
	_strip.begin(pwmTimerOf(LED_STRIP_SLOT), pwmChannelOf(LED_STRIP_SLOT),
	             pwmMux(LED_STRIP_SLOT).complementary);

	  _cmd.segment.r = 0;
	  _cmd.segment.g = 255;
	  _cmd.segment.b = 0;
	  _cmd.segment.low = 0;
	  _cmd.segment.high = 50;

	  _strip.applyCommand(_cmd);
	  osDelay(1);
	  _strip.setBrightness(70);
	  _strip.tick();

}

void LedsThread::loop(){
//	_cmd.system = 0;
//	_cmd.mode = 0;
//	_cmd.emergency_global = 1;
//	_cmd.emergency_motors = 0;
//
//	switch (_cmd.system) {
//	case 0:
//		_cmd.segment.r = 147;
//		_cmd.segment.g = 0;
//		_cmd.segment.b = 211;
//		_cmd.segment.low = 0, _cmd.segment.high = 50;
//		break; // NAV: Pink
//	case 1:
//		_cmd.segment.r = 255;
//		_cmd.segment.g = 1401000;
//		_cmd.segment.b = 0;
//		_cmd.segment.low = 51, _cmd.segment.high = 100;
//		break; // HD: Yellow
//	case 2:
//		_cmd.segment.r = 0;
//		_cmd.segment.g = 255;
//		_cmd.segment.b = 0;
//		_cmd.segment.low = 0, _cmd.segment.high = 50;
//		break; // DRILL: Green
//	case 3:
//		_cmd.segment.r = 20;
//		_cmd.segment.g = 56;
//		_cmd.segment.b = 50;
//		_cmd.segment.low = 51, _cmd.segment.high = 100;
//		break; // Avionics: Turquoise
//	}
//
//	if (_cmd.mode == 4) {
//		_cmd.segment.r = 100;
//		_cmd.segment.g = 81;
//		_cmd.segment.b = 50;
//		_cmd.segment.low = 0;
//		_cmd.segment.high = 50; // AMBER
//	}
//
//	//emergency shutdown
//	if (_cmd.mode == 5) {
//		_cmd.segment.r = 255;
//		_cmd.segment.g = 0;
//		_cmd.segment.b = 0;
//		_cmd.segment.low = 0, _cmd.segment.high = 100;
//	}
//
//	strip->applyCommand(_cmd);
//	osDelay(1);
//	strip->tickOneSystem(_cmd.system);
//	osDelay(1);


	while (popCommand(_req)) {

//		strip->clear();

		_cmd.system = _req.system;
		_cmd.mode = _req.mode;
		_cmd.emergency_global = 1;
		_cmd.emergency_motors = 0;

		switch (_cmd.system) {
			case 0: _cmd.segment.r = 147; _cmd.segment.g = 0;   _cmd.segment.b = 211; _cmd.segment.low = 0, _cmd.segment.high= 25; break; // NAV: Pink
			case 1: _cmd.segment.r = 255; _cmd.segment.g = 140; _cmd.segment.b = 0;   _cmd.segment.low = 26, _cmd.segment.high=50; break; // HD: Yellow
			case 2: _cmd.segment.r = 0;   _cmd.segment.g = 255; _cmd.segment.b = 0;   _cmd.segment.low = 51, _cmd.segment.high=75; break; // DRILL: Green
			case 3: _cmd.segment.r = 20; _cmd.segment.g = 56; _cmd.segment.b = 50; _cmd.segment.low = 76, _cmd.segment.high=100; break; // Avionics: Turquoise
		}

		if (_cmd.mode == 4) {
			_cmd.segment.r = 100; _cmd.segment.g = 81; _cmd.segment.b = 50; _cmd.segment.low = 0; _cmd.segment.high = 100; // AMBER
			for (int i = 0; i < MAX_SYSTEMS; i++)
			{
				_cmd.system = i;
				_strip.applyCommand(_cmd);
			}
		}	//emergency shutdown
		else if (_cmd.mode == 5) {
			_cmd.segment.r = 255; _cmd.segment.g = 0;   _cmd.segment.b = 0; _cmd.segment.low = 0, _cmd.segment.high= 100;
			for (int i = 0; i < MAX_SYSTEMS; i++) {
				_cmd.system = i;
				_strip.applyCommand(_cmd);
			}
		}
		else if (_cmd.mode == 6) {
			for (int i = 0; i < MAX_SYSTEMS; i++) {
				_cmd.system = i;
				_strip.applyCommand(_cmd);
			}
		}
		else
		{
//			_strip.clear();
			_strip.applyCommand(_cmd);
		}


	}

//	if (_cmd.mode == 4 || _cmd.mode == 5 || _cmd.mode == 6)
//		_strip.tickOneSystem(_cmd.system);
//	else
		_strip.tick();
}






