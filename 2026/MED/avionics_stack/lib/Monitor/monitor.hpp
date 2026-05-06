/**
 * @file monitor.hpp
 * @author Eliot Abramo
*/
#ifndef MONITOR_CORE_MONITOR_HPP_
#define MONITOR_CORE_MONITOR_HPP_

#include <Arduino.h>
#include <stdarg.h>

class SerialMonitor {
public:
    /**
     * @brief Constructor allows setting a custom baud rate (default 115200) 
     * 
     * @param baudRate
     * @return null
     */
    SerialMonitor(unsigned long baudRate = 115200);

    /**
     * @brief Initialize the serial communication
     * @return null
     */
    void begin();
    
    /**
     * @brief Log a simple string message
     * 
     * @param message 
     */
    void log(const String &message);
    
    /**
     * @brief Log a debug message with a prefix.
     * 
     * @param message 
     */
    void debug(const String &message);

    /**
     * @brief Log an error message with a prefix.
     * 
     * @param message 
     */
    void error(const String &message);
    
private:
    unsigned long baudRate;
};

#endif /* MONITOR_CORE_MONITOR_HPP_ */