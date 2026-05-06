/**
 * @file monitor.cpp
 * @author Eliot Abramo
*/
#include "monitor.hpp"

SerialMonitor::SerialMonitor(unsigned long baudRate) : baudRate(baudRate) {}

// Initialize the serial communication.
void SerialMonitor::begin() {
    Serial.begin(baudRate);
    while (!Serial) {
        log("Connected")
        ; // Wait for the serial port to connect (necessary for some boards)
    }
}

// Log a simple string message.
void SerialMonitor::log(const String &message) {
    Serial.println(message);
}

// Log a debug message with a prefix.
void SerialMonitor::debug(const String &message) {
    Serial.print("[DEBUG] ");
    Serial.println(message);
}

// Log an error message with a prefix.
void SerialMonitor::error(const String &message) {
    Serial.print("[ERROR] ");
    Serial.println(message);
}
