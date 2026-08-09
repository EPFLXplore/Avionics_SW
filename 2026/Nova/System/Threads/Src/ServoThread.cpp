#include <ServoThread.h>

ServoThread::ServoThread(const char* name, osPriority priority)
    : MessageThread(name, priority)
{
    setDelay(0);  // blocking done inside loop() via waitCommand
    // Device ids are NOT bound here: see init().
}

ServoThread::~ServoThread()
{
    // servo[] are plain member objects: nothing to free.
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
    // off servo[].global_id.
    bindDevices(servo, profile().servo);

    // servo[] are already constructed (member objects, built when this thread
    // was constructed at runtime in System::init, after HAL), but their CCRs
    // are still at CubeMX's Pulse = 0: the channel is running with no pulse at
    // all, so the servo is unpowered and free to drift until the first command.
    //
    // Home every servo through the same path a ServoRequest{go_to_zero = true}
    // takes, so boot position and commanded zero can never disagree. Channels
    // the profile left empty are constructed inert and no-op inside zero().
    for (uint8_t ch = 0; ch < SERVO_CHANNEL_COUNT; ch++)
        servo[ch].zero();
}

void ServoThread::loop()
{
    // Every channel drives a POSITIONAL servo: the angle written to the CCR is
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
            if (PWMDriver* s = deviceFor(servo, req.id)) {
                if (req.go_to_zero) s->zero();
                else                s->set_angle((float)req.angle);
            }
        } while (this->popCommand(req));
    }
}
