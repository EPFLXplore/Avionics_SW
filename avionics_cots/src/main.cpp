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


static Servo_Driver servo1;
static Servo_Driver servo2;

static ADS1234Thread mass_thread;

void setup() {
  Serial.begin(115200);
  // mass_thread.init();
  servo1.init(1, 13, 3);
}


void loop() {
      // Check if data is available
      if (Serial.available() > 0)
      {
        // Read the incoming data into a buffer
        char buffer[64];
        int len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[len] = '\0'; // Null-terminate the string      
        delay(500);
        servo1.handle_servo(buffer);
        delay(500);
    }
  delay(500);
}

