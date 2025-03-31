/**
 * @file LED_controller.cpp
 * @author Oliver Daoud
*/

#include "LED_controller.hpp"

Led_driver::Led_driver(int8_t ledPin){
    strip=new LEDStrip(ledPin, NUM_LEDS);;
    commands=nullptr;;
    new_com=0;
    pin=ledPin;
    timerFlag=false;
    timer= NULL;
}

Led_driver::~Led_driver()
{
    delete strip;
    strip = nullptr;
    delete commands;
    commands = nullptr;
}

volatile bool Led_driver::timerFlag = false;

void IRAM_ATTR Led_driver::onTimer() {
    timerFlag = !timerFlag;
}

void Led_driver::init(){

    // init only one segment
    commands = new Command();
    commands->system = 0;
    commands->mode = 5;

    commands->segment.low = 0;
    commands->segment.high = 100;
    commands->segment.r = 127;
    commands->segment.g = 0;
    commands->segment.b = 255;

    pinMode(pin, OUTPUT);
    strip->begin();   

    //init timer 
    // Initialize Timer (Timer 0, Divider 80 -> 1 tick = 1µs)
    timer = timerBegin(0, 80, true);  // Timer 0, prescaler 80 (1µs tick), count up

    // Attach ISR
    timerAttachInterrupt(timer, &onTimer, true); // Edge-triggered

    // Set timer alarm (1000000 µs = 1 second)
    timerAlarmWrite(timer, 1000000, true);  // Auto-reload enabled

    // Enable the timer alarm
    timerAlarmEnable(timer);
}

void Led_driver::handle_packet(char buffer[64]){

    buffer[16] = '\0'; // Null-terminate the string
    // Parse the command from the buffer
    int low, high, system, mode;
    int parsed = sscanf(buffer, "%d",  &mode);

    // Check if the parsing was successful
    if (parsed >= 1) {
      // Update the global command variables
      new_com = mode;

      // Debug print to verify the parsed values
      //Serial.printf("Parsed values - Start: %d, End: %d, Mode: %d, R: %d, G: %d, B: %d\n", low, high, system, mode);
      Serial.printf(" mode %d \n",mode);
    } else {
        Serial.println("Failed to parse command");
    }
}

void Led_driver::update_leds(){
    if (timerFlag){
        execute_strip();
      }
      else{
        strip->disable(0);
      }
}

void Led_driver::execute_strip(){
    commands->mode=new_com;
    //Serial.printf("executing\n");
  
    Command* current_command = commands;

    uint8_t mode = current_command->mode;
    int start = (current_command->segment.low * NUM_LEDS) / 100;
    int end = (current_command->segment.high * NUM_LEDS) / 100;
    
    uint8_t r = current_command->segment.r;
    uint8_t g = current_command->segment.g;
    uint8_t b = current_command->segment.b;

    switch (mode) {
        // On
        case 0:
            strip->mode0(start,end,r,g,b);
            break;

        // Manual
        case 1:
            strip->mode1(start,end,r,g,b,4,10,50);
            break;

        // Manual Direct
        case 2:
            strip->mode2(start,end,0,0,0,20);
            strip->mode2(start,end,r,g,b,20);
        break;
      
        // Manual Inverse
        case 3:
            strip->mode3(start,end,r,g,b);
        break;
      
        // Auto
        case 4:
            strip->mode4(start,end,r,g,b,50);
            break;

        // Off
        case 5:
            strip->mode5(start,end);
            break;

        case 6:
            strip->mode6(start,end,r,g,b,10,50,1000);

        default:
            break;
    }
}