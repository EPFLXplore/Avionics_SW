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

    // Continuous-rotation servos read zero_pulse_us as "stop", not a position,
    // so they need no auto-stop arming here - they are already stopped.
}

void ServoThread::loop()
{
    const TickType_t now        = xTaskGetTickCount();
    const TickType_t STOP_DELAY = pdMS_TO_TICKS(600);

    // Apply one command to one servo, arming auto-stop for the service modules.
    auto apply = [&](uint8_t id, const ServoRequest& r) {
        if (r.go_to_zero) servo[id].zero();
        else              servo[id].set_angle((float)r.angle);

        if (id == LEFT_SERVICE_MODULE || id == RIGHT_SERVICE_MODULE) {
            _last_cmd_tick[id] = now;
            _stop_pending[id]  = true;
        }
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
                // drive in opposite directions. 90 (stop) maps to itself.
                // go_to_zero is unaffected: zero() homes both to the same stop.
                ServoRequest mir = req;
                mir.angle = 180 - req.angle;
                apply(RIGHT_SERVICE_MODULE, mir);
            } else if (req.id <= 3) {
                apply(req.id, req);
            }
        } while (this->popCommand(req));
    }

    // Auto-stop continuous-rotation servos 1 s after last command
    for (uint8_t id = LEFT_SERVICE_MODULE; id <= RIGHT_SERVICE_MODULE; id++) {
        if (_stop_pending[id] && (now - _last_cmd_tick[id]) >= STOP_DELAY) {
            servo[id].set_angle(90.0f);
            _stop_pending[id] = false;
        }
    }
}
