#include <ServoThread.h>

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
    }
}
