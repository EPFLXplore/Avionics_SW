/**
 * @file LED_Driver.hpp
 * @author Eliot Abramo
*/
#ifndef LED_DRIVER_HPP
#define LED_DRIVER_HPP

#include <iostream>

class LED_Driver{
public:

    LED_Driver(uint32_t LED_PIN, uint32_t NUM_LEDS, uint32_t low, uint32_t high, uint8_t system, uint32_t mode);
    ~LED_Driver();

    /**
     * @brief Initialize leds 
     * 
     * @return null 
     */
    void init_LEDS();

     /**
     * @brief recieve led info
     * 
     * @return null 
     */
    void recieve_LED_packet();

     /**
     * @brief update LED
     * 
     * @return null 
     */
    void update_LEDS();

private:
    uint32_t _LED_PIN;
    uint32_t _NUM_LEDS;

    uint32_t _low;
    uint8_t _high;
    uint8_t _r;
    uint8_t _g;
    uint8_t _b;

    uint8_t _system;
    uint8_t _mode;

    LED_Driver* nav_strip;
    LED_Driver* hd_strip;
    LED_Driver* drill_strip;
    
};

#endif /** LED_DRIVER_HPP */