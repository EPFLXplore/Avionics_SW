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
	//Set ID as the dummy packet
	heartbeat().setID(Board_MasterId());

	// Bring-up order below is HEAD's, spelled out rather than switched on:
	// tick rates first, then the link, then the workers. Nothing here is latched
	// anywhere earlier - each hasDevices() is a live profile read taken at this
	// point, exactly as HEAD's `switch (Board_MasterId())` was.
	if (mass().hasDevices()) mass().setDelay(100); //ms
	if (leds().hasDevices()) leds().setDelay(80);  //ms: matches CLEANED_LEDS (tick() rate = animation speed)

	// The comms link runs on every board, and comes up before any worker so the
	// wire has an owner before anything can push to it.
	comms().start();

	// Which threads run follows from what the board actually carries. We ask
	// each thread, never the board profile: System deals in threads, threads
	// deal in devices. A thread with nothing bound is simply never started.
	if (servo().hasDevices()) servo().start(); // servo: 0 (set in ctor) - blocks in waitCommand()
	if (mass().hasDevices())  mass().start();

	// The strip has no id (LEDRequest carries none), but whether it is fitted is
	// still a profile fact - and the same one that keeps TIM15 clear for it, see
	// noTim15Conflict() in ServoConfigs.h.
	if (leds().hasDevices()) leds().start();
}
