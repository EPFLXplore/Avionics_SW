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
#define SERVO_PIN 13

class Servo_Driver
{
public:
    /**
     * @brief Construct new servo driver object, 
     */
    Servo_Driver();
    /**
     * @brief destroy the servo driver object, 
     */
    ~Servo_Driver();

    /**
     * @brief initialize a servo object and zero in
     * Note: 'ch' is servo id, 1 for drill and 2 for camera
     * @param ch 
     * @param servoPin 
     * @return null
     */
    void init(int8_t ch, int8_t servoPin);


    /**
     * @brief set a servo object to desired angle
     * @param angle 
     * @param ch 
     * @return null
     */
    void set_servo(float angle, int8_t ch);

    /**
     * @brief set a servo object to predefined zero position
     * @param ch 
     * @return null
     */
    void zero_in(int8_t ch);

    /**
     * @brief parse and handle a servo rotation request from a given serial buffer
     * @param buffer 
     * @return null
     */
    void handle_servo(char buffer[64]);

    /**
     * @brief convert angle to duty cycle for use with pwm
     * Note: 'ch' is needed as minimum and maximum angles/pulses can be different for each servo (hardcoded in the constructor)
     * @param angle
     * @param ch 
     * @return float
     */
    float angle_to_duty(float angle, int8_t ch);

    /**
     * @brief split a string according to a given delimiter
     * @param s
     * @param delimiter
     * @return vector<string>
     */
    std::vector<std::string> split(std::string s, std::string delimiter);

private:
    uint8_t _channel;
    ledc_channel_config_t* _ledc;

    uint16_t _PWM_Pin;
    uint32_t _period = DEFAULT_PWM_PERIOD;
    
    bool _zero_in;

    float min_angle[2];
	float max_angle[2];
	float min_pulse[2];
	float max_pulse[2];

    float zero_pulse[2];

    ServoRequest* servoRequest;
    ServoResponse* servoResponse;

    int8_t _packetSize;
};

#endif