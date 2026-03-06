#include "servoThreadnew.h"
#include "Timers.h" // Ensure htim2 is accessible

ServoThread::ServoThread() : MessageThread("ServoThread", osPriorityNormal, 2048) {
    servo_pwm = nullptr;
}

ServoThread::~ServoThread() {
    if (servo_pwm != nullptr) {
        delete servo_pwm;   // Free the PWMDriver memory
        servo_pwm = nullptr;
    }
}

void ServoThread::init() {
    // Initialize PWMDriver for TIM2, Channel 1 on PA0
    // The driver internally calls HAL_TIM_PWM_Start
    servo_pwm = new PWMDriver(&htim2, 1, GPIO_PIN_0, GPIOA);
}

void ServoThread::set_angle(float angle) {
    if (servo_pwm == nullptr) return;

    // Clamp angle
    if (angle < MIN_ANGLE) angle = MIN_ANGLE;
    if (angle > MAX_ANGLE) angle = MAX_ANGLE;

    // Manual Lerp: map angle to duty cycle
    float duty = MIN_DUTY + (angle - MIN_ANGLE) * (MAX_DUTY - MIN_DUTY) / (MAX_ANGLE - MIN_ANGLE);

    servo_pwm->set_pwm(duty);
}

void ServoThread::loop() {
    ServoRequest req;
    if (this->popCommand(req)) {
        if (req.zero_in) {
            set_angle(90.0f); // Center
        } else {
            // Treat 'increment' as the absolute target angle from Micro-ROS
            set_angle((float)req.increment);
        }
    }
    osDelay(20);
}
