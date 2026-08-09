#include <ServoThread.h>

ServoThread::ServoThread(const char* name, osPriority priority)
    : MessageThread(name, priority)
{
    setDelay(0);  // blocking done inside loop() via waitCommand
    // DeviceType ids are NOT bound here: see init().
}

ServoThread::~ServoThread()
{
    // _servo[] are plain member objects: nothing to free.
}

void ServoThread::init()
{
    // Bind local channels to global device ids. HERE, not in the constructor:
    // init() only runs because Thread::start() was called, which only happened
    // because System asked hasDevices() - and hasDevices() answers off the
    // profile directly, so it never needed the binding. Keeping the read in task
    // context means nothing about the board identity is latched before the
    // scheduler is up.
    //
    // This is the ONLY place the board profile is bound; everything else works
    // off _servo[].globalId.
    bindSlots(_servo, DeviceType::Servo, ConnType::Pwm0);

    // NOTHING IS DRIVEN HERE, deliberately.
    //
    // _servo[] are already constructed (member objects, built when this thread
    // was constructed in System::init, after HAL), so their timers run - but the
    // CCRs are still at CubeMX's Pulse = 0, which is no pulse at all. A servo
    // with no pulse is unpowered: it holds nothing and is free to drift. That is
    // safe HERE specifically - nothing on these channels carries enough load to
    // backdrive a servo and damage it, so an unpowered boot costs a little drift
    // and nothing else. On a mechanism that could fall under its own weight this
    // would be the wrong default.
    //
    // Homing here would mean driving to SERVO_ZERO_DEG, the compiled FALLBACK,
    // before servo_cal.yaml has been replayed - so every boot would slam the
    // hardware to a position the RPi is about to correct. Nothing moves until
    // something asks: the first ServoRequest, or a go_to_zero once Nexus has
    // sent the real home angles on link-up.
}

void ServoThread::loop()
{
    // Every channel drives a POSITIONAL _servo: the angle written to the CCR is
    // a commanded position, held until the next command. Nothing rewrites it
    // behind the operator's back - the service modules used to be auto-stopped
    // back to 90 deg 600 ms after each command, which is correct for a
    // continuous-rotation servo (90 = neutral "stop" pulse) but on a positional
    // one just snapped it home again.
    // Block for a command, then drain and execute the whole queue. Commands for
    // devices this board does not carry are popped and dropped.
    ServoRequest req;
    if (this->waitCommand(req, pdMS_TO_TICKS(10))) {
        do {
            if (PWMDriver* s = deviceFor(_servo, req.id)) {
                if (req.change_zero) s->setZeroPulseUs(angleToPulseUs((float)req.zero));
                else if (req.go_to_zero) s->zero();
                else                     s->setAngle((float)req.angle);
            }
        } while (this->popCommand(req));
    }
}
