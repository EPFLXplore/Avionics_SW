#include <ServoThread.h>
#include <initializer_list>

ServoThread::ServoThread(const char* name, osPriority priority)
    : MessageThread(name, priority), servo{}  {}

ServoThread::~ServoThread()
{
    for (int i = 0; i < 4; i++) {
        delete servo[i];
        servo[i] = nullptr;
    }
}

void ServoThread::init()
{
    servo[FRONT_CAM] = new PWMDriver(SERVO_0_CFG); //front camera
    servo[LEFT_SERVICE_MODULE] = new PWMDriver(SERVO_1_CFG); //right side servicxe module
    servo[RIGHT_SERVICE_MODULE] = new PWMDriver(SERVO_2_CFG); //left side servicxe module
    servo[3] = new PWMDriver(SERVO_3_CFG); //
}

void ServoThread::loop()
{
    ServoRequest req;
    if (this->popCommand(req)) {
        if (req.id > 3) return;

        if (req.zero_in) {
            servo[req.id]->zero();
        } else {
            servo[req.id]->set_angle((float)req.increment);
        }

        switch (req.id) {
            case LEFT_SERVICE_MODULE:
            case RIGHT_SERVICE_MODULE:
                _last_cmd_tick[req.id] = xTaskGetTickCount();
                _stop_pending[req.id]  = true;
                break;
            default:
                break;
        }
    }

    // Auto-stop continuous-rotation servos 1 s after last command
    const TickType_t STOP_DELAY = pdMS_TO_TICKS(1000);
    for (uint8_t id : {(uint8_t)LEFT_SERVICE_MODULE, (uint8_t)RIGHT_SERVICE_MODULE}) {
        if (_stop_pending[id] && (xTaskGetTickCount() - _last_cmd_tick[id]) >= STOP_DELAY) {
            servo[id]->set_angle(90.0f);
            _stop_pending[id] = false;
        }
    }
}
