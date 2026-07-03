/*
 * System.cpp
 *
 *  Created on: Feb 27, 2025
 *      Author: pcsal
 */


#include "System.h"
#include "cmsis_os2.h"
#include "main.h"
#include "Bridge.h"   // Board_MasterId()


/*
 * Per-master profile. ONE firmware runs on every master board; the 2-bit board id
 * from the PB4/PB5 straps (Board_MasterId(), in Bridge.cpp) selects which
 * subsystems come up:
 *   id 0 : servo master  (TIM15_CH1 drives a servo) + load cells
 *   id 3 : LED master    (TIM15_CH1 drives the WS2812 strip via DMA)
 *   id 1,2 : reserved (comms only for now)
 * The same id drives the USB serial ("NOVA<id>") so udev can name the port.
 */

SerialThread& System::comms()     { static SerialThread commsThread{"Comms",       osPriorityHigh};        return commsThread; }
ServoThread&  System::servo()     { static ServoThread  servoThread{"ServoThread", osPriorityAboveNormal}; return servoThread; }
MassThread&   System::mass()      { static MassThread   massThread {"MassThread",  osPriorityNormal};      return massThread; }
LedsThread&   System::leds()      { static LedsThread   ledsThread {"LedsThread",  osPriorityNormal};      return ledsThread; }
HeartBeat&    System::heartbeat() { static HeartBeat    beatThread {"HeartBeat",   osPriorityLow};         return beatThread; }

void System::init(){

	comms().setDelay(1);       //ms: poll USB RX ring + drain status queues
	heartbeat().setDelay(100); //ms: ~2 Hz liveness beat (runs on every board)

	// Construct every thread now (single-threaded, post-HAL) so later access from
	// the wire-owner never triggers lazy construction on another task.
	(void)servo();
	(void)mass();
	(void)leds();

	// Liveness runs regardless of board profile, alongside the comms link.
	heartbeat().start();

	switch (Board_MasterId()) {

	case 0: // servo master: actuators + load cells
		mass().setDelay(100); //ms
		// servo: 0 (set in ctor) - blocks in waitCommand()
		comms().start();
		//servo().start();
		mass().start();
		break;

	case 3: // LED master: WS2812 strip on TIM15_CH1 + DMA, servos on TIM1/TIM2
		leds().setDelay(80); //ms: matches CLEANED_LEDS (tick() rate = animation speed)
		comms().start();
		leds().start();
		// Servos 0/1 (TIM15) are constructed inert on this board - the strip
		// owns TIM15 (see servoTimerFree in ServoConfigs.h); 2/3 run normally.
		// Disabled while bringing up the LEDs:
		//servo().start();
		break;

	default: // reserved ids 2,1: bring up only the comms link
		comms().start();
		break;
	}

}
