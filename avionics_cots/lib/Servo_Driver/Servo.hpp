/**
 * @file Servo.hpp
 * @author Eliot Abramo
*/
#ifndef SERVO_HPP
#define SERVO_HPP


#include <iostream>
#include <ESP32Servo.h>
#include "driver/ledc.h"

#define DEFAULT_PWM_PERIOD 20000-1

class Servo_Driver
{
public:
    // Servo_Driver(ledc_channel_config_t* ledc, uint8_t channel, uint16_t PWM_Pin);
    Servo_Driver();
    ~Servo_Driver();

    void init();

    void set_servo(float angle);
    void handle_request(float pwm);
    float angle_to_duty(float angle);

private:
    uint8_t _channel;
    ledc_channel_config_t* _ledc;

    uint16_t _PWM_Pin;
    uint32_t _period = DEFAULT_PWM_PERIOD;

    float min_angle = 0;
	float max_angle = 360;
	float min_duty = 5;
	float max_duty = 10;
};

#endif