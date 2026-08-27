/*
 * LedsTask.cpp
 *
 *  Created on: Apr 8, 2026
 *      Author: pedro
 */

#include <LedsThread.h>
#include "stm32g4xx.h"

#include "Pins.h"



LedsThread::LedsThread(const char* name, osPriority priority) : MessageThread(name, priority) {}

LedsThread::~LedsThread(){}


void LedsThread::init(){
	// Same lookup a servo on this slot would do: a strip and a servo on one slot
	// are the same physical output, so the timer and channel come from one
	// table. The slot itself is LED_STRIP_SLOT, never a literal - move the strip
	// there and here follows, including the driver's DMA request.
	_strip.begin(pwmTimerOf(LED_STRIP_SLOT), pwmChannelOf(LED_STRIP_SLOT),
	             pwmConfigOf(LED_STRIP_SLOT).complementary);

	_strip.tick(); //tick whats set at begin.
}

/* Color definitions */
static constexpr Color NAV_VIOLET      = { 147,   0, 211 };
static constexpr Color HD_ORANGE       = { 255, 140,   0 };
static constexpr Color DRILL_GREEN     = {   0, 255,   0 };
static constexpr Color AVIONICS_TURQ   = {  64, 224, 208 };
static constexpr Color EMERGENCY_AMBER = { 100,  81,  50 };
static constexpr Color EMERGENCY_RED   = { 255,   0,   0 };


void LedsThread::loop(){

	while (popCommand(_req)) {

		_cmd.system = _req.system;
		_cmd.mode = _req.mode;

		switch (_cmd.system) {
			// Percentages, not pixel counts: pctToIdx floors pct*72/100, so these are
			// the values that land on the intended LED boundaries. The split is
			// 17 / 18 / 19 / 18 LEDs -> px 0-16, 17-34, 35-53, 54-71.
			case 0: _cmd.segment.r = NAV_VIOLET.r;    _cmd.segment.g = NAV_VIOLET.g;    _cmd.segment.b = NAV_VIOLET.b;    _cmd.segment.low = 0, _cmd.segment.high= 23; break; // NAV: Violet
			case 1: _cmd.segment.r = HD_ORANGE.r;     _cmd.segment.g = HD_ORANGE.g;     _cmd.segment.b = HD_ORANGE.b;     _cmd.segment.low = 24, _cmd.segment.high=48; break; // HD: Orange
			case 2: _cmd.segment.r = DRILL_GREEN.r;   _cmd.segment.g = DRILL_GREEN.g;   _cmd.segment.b = DRILL_GREEN.b;   _cmd.segment.low = 49, _cmd.segment.high=74; break; // DRILL: Green
			case 3: _cmd.segment.r = AVIONICS_TURQ.r; _cmd.segment.g = AVIONICS_TURQ.g; _cmd.segment.b = AVIONICS_TURQ.b; _cmd.segment.low = 75, _cmd.segment.high=99; break; // Avionics: Turquoise
		}

		if (_cmd.mode == 4) {
			_cmd.segment.r = EMERGENCY_AMBER.r; _cmd.segment.g = EMERGENCY_AMBER.g; _cmd.segment.b = EMERGENCY_AMBER.b; _cmd.segment.low = 0; _cmd.segment.high = 100; // AMBER
			applyToEverySystem();
		}	//emergency shutdown
		else if (_cmd.mode == 5) {
			_cmd.segment.r = EMERGENCY_RED.r; _cmd.segment.g = EMERGENCY_RED.g; _cmd.segment.b = EMERGENCY_RED.b; _cmd.segment.low = 0, _cmd.segment.high= 100;
			applyToEverySystem();
		}
		else if (_cmd.mode == 6) {
			applyToEverySystem();
		}
		else
		{
			_strip.applyCommand(_cmd);
		}


	}

	_strip.tick();
}

void LedsThread::applyToEverySystem() {
	for (int i = 0; i < MAX_SYSTEMS; i++) {
		_cmd.system = i;
		_strip.applyCommand(_cmd);
	}
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////--CHECKS--DOWN--///////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/* Checks for current: max brightness on the whole strip for the whole length may bee too much, test before raising safety threshold */
static_assert(pixelLoad(NAV_VIOLET)      <= PIXEL_BUDGET, "NAV colour over the supply budget");
static_assert(pixelLoad(HD_ORANGE)       <= PIXEL_BUDGET, "HD colour over the supply budget");
static_assert(pixelLoad(DRILL_GREEN)     <= PIXEL_BUDGET, "DRILL colour over the supply budget");
static_assert(pixelLoad(AVIONICS_TURQ)   <= PIXEL_BUDGET, "AVIONICS colour over the supply budget");
static_assert(pixelLoad(EMERGENCY_AMBER) <= PIXEL_BUDGET, "emergency-motors colour over the supply budget");
static_assert(pixelLoad(EMERGENCY_RED)   <= PIXEL_BUDGET, "emergency-shutdown colour over the supply budget");
