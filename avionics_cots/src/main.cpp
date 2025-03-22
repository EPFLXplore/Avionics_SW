/**
 * @file main.cpp
 * @author Eliot Abramo
*/
#include <iostream>
#include <Arduino.h>
#include <ESP32Servo.h>
#include "ADS1234.hpp"
#include "ADS1234_Mass_Thread.hpp"
#include "Servo.hpp"




static const int servoPin = 13;
static Servo_Driver servo1;
// static Servo_Driver servo2;

static ADS1234Thread mass_thread;

void setup() {
  // mass_thread.init();
  servo1.init();
}

void loop() {
  // mass_thread.loop();
  servo1.set_servo(180);

}

