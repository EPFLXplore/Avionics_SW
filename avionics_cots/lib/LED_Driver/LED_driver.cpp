#include "LED_driver.hpp"
#include "LEDStrip.h"

//static Command* commands[3] = {nullptr, nullptr, nullptr};
static Command* commands = nullptr;
uint8_t new_com = 0;

hw_timer_t* timer = NULL;  // Hardware timer object
volatile bool toggleFlag = false;  // Flag for ISR (avoid Serial inside ISR)

LED_Driver::LED_Driver(uint32_t LED_PIN, uint32_t NUM_LEDS, uint32_t low, uint32_t high, uint8_t system, uint32_t mode) : 
    _low(low), _high(high), _system(system), _mode(mode) {

    LEDStrip* strip = new LEDStrip(_LED_PIN, _NUM_LEDS);
    strip->begin();
}

void execute_strip() {

    commands->mode = new_com;
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
        strip->mode0(start, end, r, g, b);
        break;

        // Manual
    case 1:
        strip->mode1(start, end, r, g, b, 4, 10, 50);
        break;

        // Manual Direct
    case 2:
        strip->mode2(start, end, 0, 0, 0, 20);
        strip->mode2(start, end, r, g, b, 20);
        break;

        // Manual Inverse
    case 3:
        strip->mode3(start, end, r, g, b);
        break;

        // Auto
    case 4:
        strip->mode4(start, end, r, g, b, 50);
        break;

        // Off
    case 5:
        strip->mode5(start, end);
        break;

    case 6:
        strip->mode6(start, end, r, g, b, 10, 50, 1000);

    default:
        break;
    }
}

void IRAM_ATTR onTimer() {
    toggleFlag = !toggleFlag;
}

void SerialHandler() {
    // Check if data is available
    if (Serial.available() > 0) {
        // Read the incoming data into a buffer
        char buffer[16];
        int len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[len] = '\0'; // Null-terminate the string

        // Parse the command from the buffer
        int low, high, system, mode;
        //int parsed = sscanf(buffer, "%d %d %d %d", &low, &high, &system, &mode);
        int parsed = sscanf(buffer, "%d", &mode);

        // Check if the parsing was successful
        if (parsed >= 1) {
            // Update the global command variables
            //commands[system]->segment.low = low;
            //commands[system]->segment.high = high;
            new_com = mode;

            // Debug print to verify the parsed values
            //Serial.printf("Parsed values - Start: %d, End: %d, Mode: %d, R: %d, G: %d, B: %d\n", low, high, system, mode);
            Serial.printf(" mode %d \n", mode);
        }
        else {
            Serial.println("Failed to parse command");
        }
    }
}

void LED_Driver::init_LEDS(){
    // Nav init
    nav_strip = new LED_Driver();
    commands = new Command();
    commands->system = 0;
    commands->mode = 5;

    commands->segment.low = 0;
    commands->segment.high = 100;
    commands->segment.r = 127;
    commands->segment.g = 0;
    commands->segment.b = 255;
    

    pinMode(_LED_PIN, OUTPUT);
    strip->begin();

    //init timer 
    // Initialize Timer (Timer 0, Divider 80 -> 1 tick = 10s)
    timer = timerBegin(0, 80, true);  // Timer 0, prescaler 80 (10s tick), count up

    // Attach ISR
    timerAttachInterrupt(timer, &onTimer, true); // Edge-triggered

    // Set timer alarm (1000000 ms = 1 second)
    timerAlarmWrite(timer, 1000000, true);  // Auto-reload enabled

    // Enable the timer alarm
    timerAlarmEnable(timer);
}


void Cosco::recieve_LED_packet(){
    SerialHandler();
}

void Cosco::update_LEDS(){
    if (toggleFlag) {
        execute_strip();
    }
    else {
        strip->disable(0);
    }
}


