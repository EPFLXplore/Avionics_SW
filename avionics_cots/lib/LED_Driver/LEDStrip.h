/** 
 * @file LEDStrip.h
 * @author Eliot Abramo 
*/ 

#ifndef __LED_STRIP_H__
#define __LED_STRIP_H__

#include <Adafruit_NeoPixel.h>
#include <vector>
#include <iostream>

class LEDStrip {
  public:
    /**
     * @brief Construct a new LEDStrip object
     * 
     * @param led_pin_ 
     * @param num_leds_ 
     */
    LEDStrip(uint8_t led_pin_, uint16_t num_leds_);

    /**
     * @brief Destroy the LEDStrip object
     */
    ~LEDStrip();

    /**
     * @brief Initializes the strip
     * @return null
     */
    void begin();

    /**
     * @brief Transmit pixel data in RAM to NeoPixels.
     * @return null
     * 
     */
    void show();

    /**
     * @brief Set the Brigtness object
     * @return null
     */
    void setBrigtness();

    /**
     * @brief Set the Pixel Color object
     * 
     * @param n: pixel number
     * @param color
     * @return null 
     */
    void setPixelColor(uint16_t n, uint32_t color);

    /**
     * @brief Convert separate red, green and blue values into a single "packed" 32-bit RGB color.
     * 
     * @param r 
     * @param g 
     * @param b 
     * @return uint32_t 
     */
    uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
    
    /**
     * @brief Clean turn off of LEDs
     * @param gradual 
     */
    void disable(bool gradual = true);

    /**
     * @brief Get the strip object
     * @return Adafruit_NeoPixel* 
     */
    Adafruit_NeoPixel* get_strip();

    /**
     * @brief Set the All LEDs to same color
     * 
     * @param start 
     * @param end 
     * @param red 
     * @param green 
     * @param blue 
     * @return null
     */
    void setAll(int start, int end, byte red, byte green, byte blue);

    /**
     * @brief On, static
     * 
     * @param start 
     * @param end 
     * @param red 
     * @param green 
     * @param blue 
     * @return null
     */
    void mode0(int start, int end, byte red, byte green, byte blue);

    /**
     * @brief Manual, 
     * Recommended settings: Eyesize = 4, SpeedDelay = 10, ReturnDelay = 50
     * 
     * @param start 
     * @param end 
     * @param red 
     * @param green 
     * @param blue 
     * @param EyeSize 
     * @param SpeedDelay 
     * @param ReturnDelay 
     * @return null
     */
    void mode1(int start, int end, byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay);

    /**
     * @brief Manual Direct
     * Recommended settings: SpeedDelay = 20
     * Important note: has to be called twice. Once put pattern (color wanted), once remove (0,0,0)
     * 
     * @param start 
     * @param end 
     * @param red 
     * @param green 
     * @param blue 
     * @param SpeedDelay 
     */
    void mode2(int start, int end, byte red, byte green, byte blue, int SpeedDelay);

    /**
     * @brief Manual Inverse
     * 
     * @param start 
     * @param end 
     * @param red 
     * @param green 
     * @param blue 
     */
    void mode3(int start, int end, byte red, byte green, byte blue);


    /**
     * @brief Auto
     * Recommended settings: SpeedDelay = 50
     * 
     * @param start 
     * @param end 
     * @param red 
     * @param green 
     * @param blue 
     * @param SpeedDelay 
     */
    void mode4(int start, int end, byte red, byte green, byte blue, int SpeedDelay);

    /**
     * @brief Inaction (Blue)
     * 
     * @param start 
     * @param end 
     */
    void mode5(int start, int end);

    /**
     * @brief Whatever action
     * Recommended settings: StrobeCount = 10, FlashDelay = 50, EndPause = 1000
     * 
     * @param start 
     * @param end 
     * @param red 
     * @param green 
     * @param blue 
     * @param StrobeCount 
     * @param FlashDelay 
     * @param EndPause 
     */
    void mode6(int start, int end, byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause);

    void setFirst(bool val){firstCommand=val;}
    bool getFirst(){return firstCommand;}

  private:
    uint8_t led_pin;
    uint16_t num_leds;
    Adafruit_NeoPixel strip;
    bool firstCommand;
    // std::vector<StripPattern*> pattern_list;
    // StripPattern* get_pattern(uint8_t index);
};

#endif // __LED_STRIP_H__