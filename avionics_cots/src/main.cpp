/**
 * @file main.cpp
 * @author Eliot Abramo
*/
#include <Arduino.h>
#include "ADS1234.hpp"
#include "ADS1234_Mass_Thread.hpp"
#include "Cosco.hpp"
static ADS1234Thread mass_thread;
static Cosco device;
void setup() {
  mass_thread.init();
  device.init_LEDS();
}

void loop() {
  mass_thread.loop();
  device.recieve_LED_packet();
  device.update_LEDS();
}