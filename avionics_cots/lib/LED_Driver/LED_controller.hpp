/**
 * @file LED_controller.hpp
 * @author Oliver Daoud
*/

#ifndef LED_HPP
#define LED_HPP


#include <Arduino.h>
#include <Wire.h>
#include "driver/uart.h"
#include <vector>
#include <Adafruit_NeoPixel.h>
#include "LEDStrip.h"


#define NUM_LEDS (21)
struct Segment{
    uint8_t low;
    uint8_t high;
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };
  

struct Command {
    Segment segment;
    uint8_t system;
    uint8_t mode;
  };




class Led_driver 
{
    public:
    
    
    
    
    /**
     * @brief construct the led driver object, 
     * @param ledPin
     */
    Led_driver(int8_t ledPin);
    /**
     * @brief destroy the led driver object, 
     */
    ~Led_driver();

    /**
     * @brief initialize an led driver object : set leds to default colour
     * and start blinking interrupt 
     * @return null
     */
    void init();


    /**
     * @brief parse serial request for led
     * @param buffer
     * @return null
     */
    void handle_packet(char buffer[64]);

    /**
     * @brief update led info
     * @return null
     */
    void update_leds();

    /**
     * @brief  led timer isr
     * @return null
     */
    static void IRAM_ATTR onTimer();
    static volatile bool timerFlag;

    private: 
    
    void execute_strip();


    LEDStrip* strip;
    Command* commands;
    uint8_t new_com;
    int8_t pin;
    hw_timer_t *timer ;  // Hardware timer object
    

};

#endif