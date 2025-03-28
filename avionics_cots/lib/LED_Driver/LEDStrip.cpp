/** 
 * @file packet_definition.hpp 
 * @author Eliot Abramo 
*/ 

#include "LEDStrip.h"

LEDStrip::LEDStrip(uint8_t led_pin_, uint16_t num_leds_) : 
  led_pin(led_pin_), num_leds(num_leds_) {
    strip = Adafruit_NeoPixel(num_leds, led_pin, NEO_GRB + NEO_KHZ800);
    strip.begin();
  };


LEDStrip::~LEDStrip() {
  // pattern = nullptr;
};

void LEDStrip::begin() {
    strip.begin(); // Initialize all pixels to 'off'
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0)); // Set to black
    }
    strip.show();
}

void LEDStrip::show() {
    strip.show();
}

void LEDStrip::setBrigtness() {
  strip.setBrightness(255);
}

void LEDStrip::setPixelColor(uint16_t n, uint32_t color) {
    strip.setPixelColor(n, color);
}

uint32_t LEDStrip::Color(uint8_t r, uint8_t g, uint8_t b) {
    return strip.Color(r, g, b);
  }

void LEDStrip::disable(bool gradual) {
  static uint32_t time = 0;
  static uint32_t off_interval = 50;
  static uint16_t i = 0;
  if (gradual) {
    if (millis() - time > off_interval) {
      time = millis();
      strip.setPixelColor(i, strip.Color(0, 0, 0));
      strip.show();
      i++;
    }
  } else {
    strip.fill(strip.Color(0, 0, 0));
    strip.show();
  }
}

Adafruit_NeoPixel* LEDStrip::get_strip() {
  return &strip;
}

void LEDStrip::setAll(int start, int end, byte red, byte green, byte blue) {
  for(int i = start; i < end; i++ ) {
    this->strip.setPixelColor(i, this->strip.Color(red, green, blue));
  }
  this->strip.show();
}

void LEDStrip::mode0(int start, int end, byte red, byte green, byte blue){
  setAll(start, end, red, green, blue);
}

void LEDStrip::mode1(int start, int end, byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay){

  setAll(start, end, 0, 0, 255);
}

void LEDStrip::mode2(int start, int end, byte red, byte green, byte blue, int SpeedDelay) {
  setAll(start, end, 0, 255, 0);

}

void LEDStrip::mode3(int start, int end, byte red, byte green, byte blue){
  setAll(start, end, 255, 0, 0);

}

void LEDStrip::mode4(int start, int end, byte red, byte green, byte blue, int SpeedDelay) {
  setAll(start, end, 0, 255, 255);
}

void LEDStrip::mode5(int start, int end){
  setAll(start, end, 255,0x00,255);
}

void LEDStrip::mode6(int start, int end, byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause){
  setAll(start, end, 255, 255, 0);
}