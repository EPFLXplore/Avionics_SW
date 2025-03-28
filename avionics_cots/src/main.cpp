/**
 * @file main.cpp
 * @author Eliot Abramo
*/
#include <iostream>
#include <string>
#include <vector>
#include <Arduino.h>
#include <Wire.h>
#include "ADS1234.hpp"
#include "ADS1234_Mass_Thread.hpp"
#include "Dust_Driver.hpp"

static ADS1234Thread mass_thread;
static Dust dust_sensor;

void setup() {
  Serial.begin(115200);
  // mass_thread.init();
  dust_sensor.init();
}


void loop() {
  if (!dust_sensor.is_alive()) {
    dust_sensor.init();
  }
  // mass_thread.loop();
  dust_sensor.loop();
}

