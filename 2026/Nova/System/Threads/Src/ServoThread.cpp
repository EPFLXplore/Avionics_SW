#include <ServoThread.h>

ServoThread::ServoThread(const char* name, osPriority priority)
    : MessageThread(name, priority)
{
    setDelay(0);  // blocking done inside loop() via waitCommand
}

ServoThread::~ServoThread()
{
    // servo[] are plain member objects: nothing to free.
}

void ServoThread::init()
{
    // servo[] are already constructed (member objects, built when this thread
    // was constructed at runtime in System::init, after HAL), but their CCRs
    // are still at CubeMX's Pulse = 0: the channel is running with no pulse at
    // all, so the servo is unpowered and free to drift until the first command.
    //
    // Home every servo through the same path a ServoRequest{go_to_zero = true}
    // takes, so boot position and commanded zero can never disagree. Servos
    // constructed inert (servoTimerFree) no-op inside zero().
    for (uint8_t id = 0; id < 4; id++)
        servo[id].zero();
}

void ServoThread::loop()
{
    // Every channel drives a POSITIONAL servo: the angle written to the CCR is
    // a commanded position, held until the next command. Nothing rewrites it
    // behind the operator's back - the service modules used to be auto-stopped
    // back to 90 deg 600 ms after each command, which is correct for a
    // continuous-rotation servo (90 = neutral "stop" pulse) but on a positional
    // one just snapped it home again.
    auto apply = [this](uint8_t id, const ServoRequest& r) {
        if (r.go_to_zero) servo[id].zero();
        else              servo[id].set_angle((float)r.angle);
    };

    // Block for a command, then drain and execute the whole queue.
    // SERVICE_MODULE_BOTH expands into both service modules in the same
    // iteration, so their CCR writes land before TIM7 can fire -> simultaneous.
    ServoRequest req;
    if (this->waitCommand(req, pdMS_TO_TICKS(10))) {
        do {
            if (req.id == SERVICE_MODULE_BOTH) {
                // Block duplicate open (0) or close (180) commands.
                const bool is_toggle = !req.go_to_zero &&
                                       (req.angle == 0 || req.angle == 180);
                if (is_toggle && req.angle == _last_both_cmd) continue;
                if (is_toggle) _last_both_cmd = req.angle;

                apply(LEFT_SERVICE_MODULE, req);

                // RIGHT is mounted opposite LEFT: mirror the angle so the two
                // reach mirrored positions. 90 (mid-travel) maps to itself.
                // go_to_zero is unaffected: zero() homes both to the same angle.
                ServoRequest mir = req;
                mir.angle = 180 - req.angle;
                apply(RIGHT_SERVICE_MODULE, mir);
            } else if (req.id <= 3) {
                apply(req.id, req);
            }
        } while (this->popCommand(req));
    }
}
