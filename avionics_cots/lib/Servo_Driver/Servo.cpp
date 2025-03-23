/**
 * @file Servo.cpp
 * @author Eliot Abramo
*/
#include "Servo.hpp"

// Servo_Driver::Servo_Driver(ledc_channel_config_t* ledc, uint8_t channel, uint16_t PWM_Pin)
//             :_ledc(ledc), _PWM_Pin(PWM_Pin), _channel(channel) {}

Servo_Driver::Servo_Driver()
{
    servoRequest = new ServoRequest();
    servoResponse = new ServoResponse();
}
Servo_Driver::~Servo_Driver()
{
    delete servoRequest;
    servoRequest = nullptr;
    delete servoResponse;
    servoResponse = nullptr;
}

std::vector<std::string> Servo_Driver::split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;
  
    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }
  
    res.push_back (s.substr (pos_start));
    return res;
  }

void Servo_Driver::init()
{    
    ledcSetup(0, 50, 16);
    ledcAttachPin(13, 0);
    zero_in(1);
}

void Servo_Driver::zero_in(int8_t ch)
{
    float duty_cycle = (zero_pulse[ch-1] * 65535) / 20000;
    ledcWrite(0, duty_cycle);
}

float Servo_Driver::angle_to_duty(float angle, int8_t ch)
{
    float pwm = map(angle, min_angle[ch-1], max_angle[ch-1], min_pulse[ch-1], max_pulse[ch-1]);
    float duty_cycle = (pwm * 65535) / 20000;
    return duty_cycle;
}

void Servo_Driver::set_servo(float angle, int8_t ch)
{
    // if (angle > max_angle)
    //     angle = max_angle;
    // if (angle < min_angle)
    //     angle = min_angle;
    
    // float duty_cycle = angle_to_duty(angle);
    
    // float pwm = (uint32_t)(_period*duty_cycle/100 - 1);
    ledcWrite(0, angle_to_duty(angle, ch));
}

void Servo_Driver::handle_servo(char buffer[64])
{
    std::string to_parse = buffer;
    std::vector<std::string> parsed = split(to_parse, ",");        
    if (parsed[0] != "servo")
    {
        Serial.println("Invalid request");
    }
    else
    {
        if (not(servoRequest == nullptr))
        {
            if (parsed.size() == 4)
            {
                servoRequest->id = stoi(parsed[1]);
                servoRequest->angle = stoi(parsed[2]);
                servoRequest->zero_in = stoi(parsed[3]);

                if (not(servoRequest->id == 1 or servoRequest->id == 2))
                {
                    Serial.println("Invalid id");
                    servoResponse->id = servoRequest->id;
                    servoResponse->angle = servoRequest->angle;
                    servoResponse->success = false;
                }
                else if (not(servoRequest->angle <= 180 and servoRequest->angle >= -180))
                {
                    Serial.println("Invalid angle");
                    servoResponse->id = servoRequest->id;
                    servoResponse->angle = servoRequest->angle;
                    servoResponse->success = false;
                }
                else if (servoRequest->zero_in == true)
                {
                    zero_in(servoRequest->id);
                    Serial.print("zeroed in servo ");
                    Serial.println(servoRequest->id);
                    servoResponse->id = servoRequest->id;
                    servoResponse->angle = 0;
                    servoResponse->success = true;
                }
                else
                {
                    set_servo(servoRequest->angle, servoRequest->id);
                    Serial.print("set servo ");
                    Serial.print(servoRequest->id);
                    Serial.print(" to ");
                    Serial.print(servoRequest->angle);
                    Serial.println(" degrees.");
                    
                    servoResponse->id = servoRequest->id;
                    servoResponse->angle = servoRequest->angle;
                    servoResponse->success = true;
                }
            }
            else
            {
                delay(1000);
            }
        }
    }
}


