#include <ServoThread.h>

extern TIM_HandleTypeDef htim7;

ServoThread::ServoThread(const char* name, osPriority priority)
    : MessageThread(name, priority), servo{}
{
    setDelay(0);  // blocking done inside loop() via waitCommand
}

ServoThread::~ServoThread()
{
    for (int i = 0; i < 4; i++) {
        delete servo[i];
        servo[i] = nullptr;
    }
}

void ServoThread::init()
{
    servo[FRONT_CAM]            = new PWMDriver(SERVO_0_CFG);
    servo[LEFT_SERVICE_MODULE]  = new PWMDriver(SERVO_1_CFG);
    servo[RIGHT_SERVICE_MODULE] = new PWMDriver(SERVO_2_CFG);
    servo[3]                    = new PWMDriver(SERVO_3_CFG);

    // Start sync master after all slaves are configured and running.
    // TIM7's first overflow resets all slave counters simultaneously,
    // phase-locking their PWM cycles from that point forward.
    HAL_TIM_Base_Start(&htim7);
}

void ServoThread::loop()
{
    const TickType_t now        = xTaskGetTickCount();
    const TickType_t STOP_DELAY = pdMS_TO_TICKS(600);

    // Apply one command to one servo, arming auto-stop for the service modules.
    auto apply = [&](uint8_t id, const ServoRequest& r) {
        if (r.zero_in) servo[id]->zero();
        else           servo[id]->set_angle((float)r.increment);

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
                const bool is_toggle = !req.zero_in &&
                                       (req.increment == 0 || req.increment == 180);
                if (is_toggle && req.increment == _last_both_cmd) continue;
                if (is_toggle) _last_both_cmd = req.increment;

                apply(LEFT_SERVICE_MODULE, req);

                // RIGHT is mounted opposite LEFT: mirror the angle so the two
                // drive in opposite directions. 90 (stop) maps to itself.
                // zero_in is unaffected — zero() homes both to the same stop.
                ServoRequest mir = req;
                mir.increment = 180 - req.increment;
                apply(RIGHT_SERVICE_MODULE, mir);
            } else if (req.id <= 3) {
                apply(req.id, req);
            }
        } while (this->popCommand(req));
    }

    // Auto-stop continuous-rotation servos 1 s after last command
    for (uint8_t id = LEFT_SERVICE_MODULE; id <= RIGHT_SERVICE_MODULE; id++) {
        if (_stop_pending[id] && (now - _last_cmd_tick[id]) >= STOP_DELAY) {
            servo[id]->set_angle(90.0f);
            _stop_pending[id] = false;
        }
    }
}
