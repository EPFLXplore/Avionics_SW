/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#pragma once
#include "MessageThread.h"
#include "LEDStrip.h"
#include "BoardProfile.h"   // ConnType, DeviceType

class LedsThread : public MessageThread<LEDRequest, EmptyMessage> {
public:
	LedsThread(const char* name, osPriority priority);
	~LedsThread();

	/** True when this board is fitted with a strip. Same question System asks
	 *  MassThread and ServoThread, so nothing above the threads reads the
	 *  profile. A strip has no id (LEDRequest carries none), so this is a plain
	 *  capability rather than a bound device. */
	bool hasDevices() const { return anySlot(DeviceType::LedStrip); }

	void init();
	void loop();

private:
	bool _cleared = false;
	LEDRequest _req{};
	LEDStrip _strip{WS2812_MAX_LEDS};          // the strip as a plain object (<= WS2812_MAX_LEDS)
	Command _cmd;
};

