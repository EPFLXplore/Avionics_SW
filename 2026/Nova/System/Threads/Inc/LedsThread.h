/*
 * TestTask.h
 *
 *  Created on: Jul 9, 2025
 *      Author: pedro
 */

#ifndef THREADS_INC_LEDSTHREAD_H_
#define THREADS_INC_LEDSTHREAD_H_

#include "MessageThread.h"
#include "LEDStrip.h"
#include "BoardProfile.h"   // profile().led_strip

class LedsThread : public MessageThread<LEDRequest, EmptyMessage> {
public:
	LedsThread(const char* name, osPriority priority);
	~LedsThread();

	/** True when this board is fitted with a strip. Same question System asks
	 *  MassThread and ServoThread, so nothing above the threads reads the
	 *  profile. A strip has no id (LEDRequest carries none), so this is a plain
	 *  capability rather than a bound device. */
	bool hasDevices() const { return profile().led_strip; }

	void init();
	void loop();

private:
	bool cleared = false;
	LEDRequest req{};
	LEDStrip strip{75};          // the strip as a plain object (<= WS2812_MAX_LEDS)
	Command cmd;
};

#endif /* THREADS_INC_LEDSTHREAD_H_ */
