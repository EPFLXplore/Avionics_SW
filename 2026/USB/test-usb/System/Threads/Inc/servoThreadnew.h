#pragma once
#include "MessageThread.h"
#include "packets.h"
#include "PWMDriver.hpp"

class ServoThread : public MessageThread<ServoRequest, EmptyMessage> {
public:
    ServoThread();
    virtual ~ServoThread(); // virtual ensures proper cleanup

    void init() override;
    void loop() override;

    // DECLARE THIS: Fixes "no declaration matches" error
    void set_angle(float angle);

private:
    // DECLARE THIS: Fixes "'servo_pwm' was not declared" error
    PWMDriver* servo_pwm;

    // Constants for mapping (Standard 50Hz Servo)
    const float MIN_ANGLE = 0.0f;
    const float MAX_ANGLE = 180.0f;
    const float MIN_DUTY = 2.5f;  // 1ms
    const float MAX_DUTY = 12.5f; // 2ms
};
