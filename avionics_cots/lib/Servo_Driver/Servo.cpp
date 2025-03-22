/**
 * @file Servo.cpp
 * @author Eliot Abramo
*/
#include "Servo.hpp"

// Servo_Driver::Servo_Driver(ledc_channel_config_t* ledc, uint8_t channel, uint16_t PWM_Pin)
//             :_ledc(ledc), _PWM_Pin(PWM_Pin), _channel(channel) {}

Servo_Driver::Servo_Driver(){}
Servo_Driver::~Servo_Driver(){}

void Servo_Driver::init()
{
    ledcSetup(0, 50, 16);
    ledcAttachPin(13, 0);
    
}

float Servo_Driver::angle_to_duty(float angle)
{
    float pwm = map(angle, 0, 360, 500, 2500);
    float duty_cycle = (pwm * 65535) / 20000;
    return duty_cycle;
}

void Servo_Driver::set_servo(float angle)
{
    // if (angle > max_angle)
    //     angle = max_angle;
    // if (angle < min_angle)
    //     angle = min_angle;
    
    // float duty_cycle = angle_to_duty(angle);
    
    // float pwm = (uint32_t)(_period*duty_cycle/100 - 1);
    ledcWrite(0, angle_to_duty(angle));
}




