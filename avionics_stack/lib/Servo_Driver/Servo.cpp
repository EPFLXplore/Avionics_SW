/**
 * @file Servo.cpp
 * @author Eliot Abramo
*/
#include "Servo.hpp"

Servo_Driver::Servo_Driver(){
    servoRequest = new ServoRequest();

    min_angle = -500;
	max_angle = 500;
	min_pulse = 400;
	max_pulse = 2600;

    zero_pulse = 1500;
    angle = 0;
}

Servo_Driver::~Servo_Driver(){
    delete servoRequest;
    servoRequest = nullptr;
}

void Servo_Driver::init(int8_t servoPin, uint8_t channel){
    _channel = channel;
    ledcSetup(channel, 50, 16);
    ledcAttachPin(servoPin, channel);

    if(channel == 0){
        // set camera
        angle = 300;
        set_servo();
    }else if(channel == 1){
        // set drill
        angle = -400;
        set_servo();
    }else{
        zero_in();
    }
}

void Servo_Driver::zero_in(){
    float duty_cycle = (zero_pulse * 65535) / 20000;
    ledcWrite(_channel, duty_cycle);
}

float Servo_Driver::angle_to_duty(){
    // Float lerp to avoid integer truncation from Arduino map()
    float spanA = (float)max_angle - (float)min_angle;
    float spanP = (float)max_pulse - (float)min_pulse;
    float pwm   = (float)min_pulse + ((angle - (float)min_angle) * spanP) / spanA;

    // Convert µs, 16-bit duty @ 50 Hz (20,000 µs period)
    float duty_cycle = (pwm * 65535.0f) / 20000.0f;
    return duty_cycle;
}


void Servo_Driver::set_servo(){
    ledcWrite(_channel, angle_to_duty());
}

std::vector<std::string> split(std::string s, std::string delimiter) {
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


void Servo_Driver::handle_servo() {
    if (servoRequest != nullptr) {
        int32_t new_angle = angle + servoRequest->increment;
        if (servoRequest->zero_in) { //check if zero_in was requested
            angle = 0;
            zero_in();
        } else if (!(new_angle <= max_angle and new_angle >= min_angle)) { //check for angle within bounds
            angle = new_angle <= min_angle ? min_angle : max_angle;
            set_servo();
        } else { //set servo to angle if OK
            angle = new_angle;
            set_servo();            
        }
    }
}

void Servo_Driver::set_request(ServoRequest req) {
    *servoRequest = req;
}
