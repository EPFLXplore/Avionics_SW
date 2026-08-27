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
pHMeterThread& System::ph()       { static pHMeterThread phThread  {"pHMeter",    osPriorityNormal};      return phThread; }
LedsThread&   System::leds()      { static LedsThread   ledsThread {"LedsThread",  osPriorityNormal};      return ledsThread; }
HeartBeatThread&    System::heartbeat() { static HeartBeatThread    beatThread {"HeartBeat",   osPriorityLow};         return beatThread; }

void System::init(){

	/* EVERY thread's tick rate, and the only place any of them is set - a rate
	 * hidden in a constructor is a rate nobody finds when the schedule needs
	 * reading. Thread's default is 100 ms, so a thread missing from this block
	 * gets that silently; keep the list complete.
	 *
	 * The rates below are unconditional. The profile-dependent ones are further
	 * down, guarded by hasDevices().
	 *
	 * ServoThread does not tick on this delay: loop() blocks in waitCommand()
	 * for up to 10 ms, so the queue drives it and a command is acted on the
	 * moment it arrives. The 1 ms is a BACKSTOP. It used to be 0, which
	 * CMSIS-RTOS2 turns into a no-op - not even a yield - so the only thing
	 * keeping the task off the CPU was the queue receive blocking. If that
	 * queue ever failed to be created, waitCommand() would return false
	 * immediately and this thread would spin flat out at osPriorityAboveNormal,
	 * starving every osPriorityNormal worker. One millisecond buys a guaranteed
	 * block on a path that otherwise has none, and costs nothing measurable: the
	 * PWM frame is 20 ms, so a command delayed 1 ms lands in the same frame. */
	comms().setDelay(1);       //ms: poll USB RX ring + drain status queues
	heartbeat().setDelay(100); //ms: ~2 Hz liveness beat (runs on every board)
	servo().setDelay(1);       //ms: a floor, not the tick - see below

	// Construct every thread now (single-threaded, post-HAL) so later access from
	// the wire-owner never triggers lazy construction on another task.
	(void)servo();
	(void)mass();
	(void)ph();
	(void)leds();

	// Liveness runs regardless of board profile, alongside the comms link.
	heartbeat().start();
	//Set ID as the dummy packet
	heartbeat().setID(Board_MasterId());

	// Bring-up order below is HEAD's, spelled out rather than switched on:
	// tick rates first, then the link, then the workers. Nothing here is latched
	// anywhere earlier - each hasDevices() is a live profile read taken at this
	// point, exactly as HEAD's `switch (Board_MasterId())` was.
	// 10 ms, NOT 100: the HX711 runs at 80 SPS (RATE=1), a sample every 12.5 ms,
	// and it overwrites its output register whether or not we read it. Polling at
	// 100 ms would take 1 sample in 8 and stretch MassThread's 160-deep buffer
	// from a 2 s window to 16 s. The poll has to be faster than the conversion.
	// Back to 10 SPS means 100 here AND AVG_SIZE 20 in MassThread.h.
	if (mass().hasDevices()) mass().setDelay(10);  //ms
	if (ph().hasDevices())   ph().setDelay(500);   //ms
	if (leds().hasDevices()) leds().setDelay(80);  //ms: matches CLEANED_LEDS (tick() rate = animation speed)

	// The comms link runs on every board, and comes up before any worker so the
	// wire has an owner before anything can push to it.
	comms().start();

	// Which threads run follows from what the board actually carries. We ask
	// each thread, never the board profile: System deals in threads, threads
	// deal in devices. A thread with nothing bound is simply never started.
	if (servo().hasDevices()) servo().start();
	if (mass().hasDevices())  mass().start();
	if (ph().hasDevices())    ph().start();

	// The strip has no id (LEDRequest carries none), but whether it is fitted is
	// still a profile fact - and the same one that keeps TIM15 clear for it, see
	// noTim15Conflict() in ServoConfigs.h.
	if (leds().hasDevices()) leds().start();
}
