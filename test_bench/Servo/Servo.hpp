/**
 * @file Servo.hpp
 * @author Eliot Abramo
*/

#ifndef SERVO_HPP
#define SERVO_HPP


#include <string>
#include <vector>
#include "main.h"
#include <iostream>

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
    void init(TIM_HandleTypeDef *htim, uint8_t channel);


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
    //void handle_servo();

    void setAngle(int32_t new_angle);


    /**
     * @brief convert angle to duty cycle for use with pwm
     * Note: 'ch' is needed as minimum and maximum angles/pulses can be different for each servo (hardcoded in the constructor)
     * @param angle
     * @return float
     */
    float angle_to_duty();

   // void set_request(ServoRequest req);

private:
    uint8_t _channel;
    TIM_HandleTypeDef *_htim = nullptr;
    
    bool _zero_in;

    float min_angle;
	float max_angle;
	float min_pulse;
	float max_pulse;

    float zero_pulse;

    int32_t angle;
    void apply();

   // ServoRequest* servoRequest;
};

#endif
