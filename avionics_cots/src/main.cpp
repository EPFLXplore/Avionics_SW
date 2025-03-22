/**
 * @file main.cpp
 * @author Eliot Abramo
*/
#include <iostream>
#include <string>
#include <vector>
#include <Arduino.h>
#include <ESP32Servo.h>
#include "ADS1234.hpp"
#include "ADS1234_Mass_Thread.hpp"
#include "Servo.hpp"
// #include "packet_definition.hpp"

using namespace std;


static const int servoPin = 13;
static Servo_Driver servo1;
// static Servo_Driver servo2;

static ADS1234Thread mass_thread;

void setup() {
  Serial.begin(115200);
  // mass_thread.init();
  servo1.init();
}


void loop() {
      // Check if data is available
      if (Serial.available() > 0)
      {
        // Read the incoming data into a buffer
        char buffer[64];
        int len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[len] = '\0'; // Null-terminate the string
        // std::string to_parse = buffer;
        // std::vector<std::string> parsed = split(to_parse, ", ");

        // // Read and store the first byte (the "first bit")
        // uint8_t firstByte = static_cast<uint8_t>(buffer[0]);
        // // // Do something with the stored value, e.g., print it
        // // Serial.print("First byte received: 0x");
        // // Serial.println(firstByte, HEX);
        // servoRequest.id = stoi(parsed[0]);
        // servoRequest.angle = stoi(parsed[1]);
        // servoRequest.zero_in = stoi(parsed[2]);
        // Serial.println(buffer);
        // // if(servoRequest.id == 0)
        // // {
        //   servo1.set_servo(servoRequest.angle, servoRequest.id);
        //   Serial.print("set servo ");
        //   Serial.print(servoRequest.id);
        //   Serial.print(" to ");
        //   Serial.print(servoRequest.angle);
        //   Serial.println(" degrees.");
        //   delay(1000);
        // // }
        delay(500);
        servo1.handle_servo(buffer);
        delay(500);
    }

  // mass_thread.loop();
  // servo1.set_servo(20, 1);
  // Serial.println("set servo to 20");
  // delay(1000);
  // servo1.set_servo(180, 1);
  // Serial.println("set servo to 180");
  // delay(1000);
  // servo1.zero_in(1);
  // Serial.println("zeroed in");
  // delay(1000);
  // servo1.set_servo(-20, 1);
  // Serial.println("set servo to -20");
  delay(500);

}

