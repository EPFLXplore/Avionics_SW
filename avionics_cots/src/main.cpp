/**
 * @file main.cpp
 * @author Eliot Abramo
*/
#include <Arduino.h>
#include "ADS1234.hpp"
#include "ADS1234_Mass_Thread.hpp"
#include "Dust_Driver.hpp"

static ADS1234Thread mass_thread;
static Dust dust_sensor;

void setup() {
  mass_thread.init();
  dust_sensor.init();
}

void loop() {
  mass_thread.loop();
  dust_sensor.loop();
}

