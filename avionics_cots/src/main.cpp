/**
 * @file main.cpp
 * @author Eliot Abramo
*/
#include <iostream>
#include <string>
#include <vector>
#include <Arduino.h>
#include <ESP32Servo.h>
#include "Servo.hpp"
#include "LED_controller.hpp"
// #include "packet_definition.hpp"
using namespace std;


static Servo_Driver servo1;
static Servo_Driver servo2;
Led_driver* led=new Led_driver(27);

void setup() {
  Serial.begin(115200);
  // mass_thread.init();
  servo1.init(1, 13,3);

  led->init();
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
    led->handle_packet(buffer);
    delay(500);
  }
  led->update_leds();
  delay(500);
}
