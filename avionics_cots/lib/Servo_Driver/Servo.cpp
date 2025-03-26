/**
 * @file Servo.cpp
 * @author Eliot Abramo
*/
#include "Servo.hpp"

Servo_Driver::Servo_Driver()
{
    servoRequest = new ServoRequest();
    servoResponse = new ServoResponse();

    min_angle[2] = {-180, -180};
	max_angle[2] = {180, 180};
	min_pulse[2] = {500, 500};
	max_pulse[2] = {2500, 2500};

    zero_pulse[2] = {1500, 1500};

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

void Servo_Driver::init(int8_t ch, int8_t servoPin, int8_t packetSize)
{    
    ledcSetup(0, 50, 16);
    ledcAttachPin(servoPin, 0);
    zero_in(ch);
    _packetSize = packetSize;
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
    ledcWrite(0, angle_to_duty(angle, ch));
}

void Servo_Driver::handle_servo(char buffer[64])
{
    std::string to_parse = buffer;
    std::vector<std::string> parsed = split(to_parse, ",");        
    if (servoRequest != nullptr)
    {
        if (parsed.size() == _packetSize)
        {
            servoRequest->id = stoi(parsed[1]); //parsed strings from the buffer placed in a vector after splitting. 
            servoRequest->angle = stoi(parsed[2]); //Casting them into the servoRequest struct for error-checking
            servoRequest->zero_in = stoi(parsed[3]);

            if (not(servoRequest->id == 1 or servoRequest->id == 2)) //servo id 1 being Drill and id 2 being Camera
            { //check for valid servo id
                Serial.println("Invalid id");
                servoResponse->id = servoRequest->id;
                servoResponse->angle = servoRequest->angle;
                servoResponse->success = false;
            }
            else if (not(servoRequest->angle <= max_angle[servoRequest->id - 1] and servoRequest->angle >= min_angle[servoRequest->id - 1]))
            { //check for angle within bounds
                Serial.println("Invalid angle");
                servoResponse->id = servoRequest->id;
                servoResponse->angle = servoRequest->angle;
                servoResponse->success = false;
            }
            else if (servoRequest->zero_in == true)
            { //check if zero_in was requested
                zero_in(servoRequest->id);
                Serial.print("zeroed in servo ");
                Serial.println(servoRequest->id);
                servoResponse->id = servoRequest->id;
                servoResponse->angle = 0; // cancel rotation and zero in
                servoResponse->success = true;
            }
            else
            { //set servo to angle if OK
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



