/**
 * @file Servo.hpp
 * @author Eliot Abramo
*/
#ifndef SERVO_HPP
#define SERVO_HPP


#include <iostream>
#include <string>
#include <vector>
#include <ESP32Servo.h>
#include "driver/ledc.h"
#include "packet_definition.hpp"

#define DEFAULT_PWM_PERIOD 20000-1

class Servo_Driver
{
public:
    // Servo_Driver(ledc_channel_config_t* ledc, uint8_t channel, uint16_t PWM_Pin);
    Servo_Driver();
    ~Servo_Driver();

    void init();

    void set_servo(float angle, int8_t ch);
    void zero_in(int8_t ch);
    void handle_servo(char buffer[64]);
    float angle_to_duty(float angle, int8_t ch);
    std::vector<std::string> split(std::string s, std::string delimiter);

private:
    uint8_t _channel;
    ledc_channel_config_t* _ledc;

    uint16_t _PWM_Pin;
    uint32_t _period = DEFAULT_PWM_PERIOD;
    
    bool _zero_in;

    float min_angle[2] = {-180, -180};
	float max_angle[2] = {180, 180};
	float min_pulse[2] = {500, 500};
	float max_pulse[2] = {2500, 2500};

    float zero_pulse[2] = {1400, 1400};

    ServoRequest* servoRequest;
    ServoResponse* servoResponse;
};

#endif