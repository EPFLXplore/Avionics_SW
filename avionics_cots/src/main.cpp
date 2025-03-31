/**
 * @file main.cpp
 * @author Eliot Abramo
*/
#include <Arduino.h>
#include "Mass_thread.hpp"

//static HX711Thread mass_thread;
HX711Thread* mass_thread = new HX711Thread();

void setup() {
	mass_thread->init();
}

void loop() {
	mass_thread->loop(); // CHANGES TO DO HERE
}