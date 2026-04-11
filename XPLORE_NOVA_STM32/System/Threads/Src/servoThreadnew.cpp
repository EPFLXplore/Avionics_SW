#include "servoThreadnew.h"

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
    servo[0] = new PWMDriver(SERVO_0_CFG);
    servo[1] = new PWMDriver(SERVO_1_CFG);
    servo[2] = new PWMDriver(SERVO_2_CFG);
    servo[3] = new PWMDriver(SERVO_3_CFG);
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
