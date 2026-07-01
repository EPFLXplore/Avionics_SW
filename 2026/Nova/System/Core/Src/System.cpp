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

SerialThread& System::comms() { static SerialThread commsThread{"Comms",       osPriorityHigh};        return commsThread; }
ServoThread&  System::servo() { static ServoThread  servoThread{"ServoThread", osPriorityAboveNormal}; return servoThread; }
MassThread&   System::mass()  { static MassThread   massThread {"MassThread",  osPriorityNormal};      return massThread; }
LedsThread&   System::leds()  { static LedsThread   ledsThread {"LedsThread",  osPriorityNormal};      return ledsThread; }

void System::init(){

	comms().setDelay(1); //ms: poll USB RX ring + drain status queues

	// Construct every thread now (single-threaded, post-HAL) so later access from
	// the wire-owner never triggers lazy construction on another task.
	(void)servo();
	(void)mass();
	(void)leds();

	switch (Board_MasterId()) {

	case 0: // servo master: actuators + load cells
		mass().setDelay(100); //ms
		// servo: 0 (set in ctor) - blocks in waitCommand()
		comms().start();
		servo().start();
		mass().start();
		break;

	case 3: // LED master: WS2812 strip on TIM15_CH1 + DMA
		leds().setDelay(20); //ms
		comms().start();
		leds().start();
		break;

	default: // reserved ids 2,1: bring up only the comms link
		comms().start();
		break;
	}

}
