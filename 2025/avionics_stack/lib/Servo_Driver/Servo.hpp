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
     * @param servoPin 
     * @return null
     */
    void init(int8_t servoPin, uint8_t channel);


    /**
     * @brief set a servo object to desired angle
     * @param angle 
     * @return null
     */
    void set_servo();

    /**
     * @brief set a servo object to predefined zero position
     * @return null
     */
    void zero_in();

    /**
     * @brief parse and handle a servo rotation request from a given serial buffer
     * @return null
     */
    void handle_servo();

    /**
     * @brief convert angle to duty cycle for use with pwm
     * Note: 'ch' is needed as minimum and maximum angles/pulses can be different for each servo (hardcoded in the constructor)
     * @param angle
     * @return float
     */
    float angle_to_duty();

    void set_request(ServoRequest req);

private:
    uint8_t _channel;
    ledc_channel_config_t* _ledc;

    uint16_t _PWM_Pin;
    uint32_t _period = DEFAULT_PWM_PERIOD;
    
    bool _zero_in;

    float min_angle;
	float max_angle;
	float min_pulse;
	float max_pulse;

    float zero_pulse;

    int32_t angle;

    ServoRequest* servoRequest;
};

#endif
