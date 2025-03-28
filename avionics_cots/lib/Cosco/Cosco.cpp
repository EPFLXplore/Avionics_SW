/**
 * @file Cosco.cpp
 * @author Eliot Abramo
*/
#include "Cosco.hpp"
#include <Wire.h>
#include <Arduino.h>

#include "ADS1234.hpp"
#include "packet_definition.hpp"
#include "packet_id.hpp"

#include "LEDStrip.h"

#define LED_PIN (27)
#define NUM_LEDS (21)


struct Segment {
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


static LEDStrip* strip = new LEDStrip(LED_PIN, NUM_LEDS);
//static Command* commands[3] = {nullptr, nullptr, nullptr};
static Command* commands = nullptr;
uint8_t new_com = 0;


hw_timer_t* timer = NULL;  // Hardware timer object
volatile bool toggleFlag = false;  // Flag for ISR (avoid Serial inside ISR)


/**********  ALL LED RELATED FUNCTIONS *************/
// Set default segments colors
void default_segments(LEDStrip* strip) {
    int segmentSize = NUM_LEDS;
    for (int i = 0; i < segmentSize; i++)
        strip->setPixelColor(i, strip->Color(127, 0, 255));
    strip->show();
    delay(500);
}


void initialize_strip() {
    // Nav init
    commands = new Command();
    commands->system = 0;
    commands->mode = 5;

    commands->segment.low = 0;
    commands->segment.high = 100;
    commands->segment.r = 127;
    commands->segment.g = 0;
    commands->segment.b = 255;

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
    //Serial.printf("Current: %d, %d, %d, %d, %d, %d \n",start,end,mode,r,g,b);
    //Serial.printf("sector %d, mode %d \n", i,mode);

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

void IRAM_ATTR onTimer() {
    toggleFlag = !toggleFlag;
}

/**********  END OF LED RELATED FUNCTIONS *************/



Cosco::Cosco()
{
    Serial.begin(115200);
    Serial.println("Serial launched");
}

Cosco::~Cosco(){}

void Cosco::sendMassConfigPacket(MassConfigPacket *configPacket)
{
    // Serialize and send MassConfigPacket
    uint8_t packetBuffer[sizeof(MassConfigPacket) + 1];
    packetBuffer[0] = MassConfig_ID; 
    memcpy(packetBuffer + 1, configPacket, sizeof(MassConfigPacket));
    Serial.write(packetBuffer, sizeof(MassConfigPacket));
}

void Cosco::sendMassConfigRequestPacket(MassConfigRequestPacket *requestPacket)
{
    // Serialize and send MassConfigRequestPacket
    uint8_t packetBuffer[sizeof(MassConfigRequestPacket) + 1];
    packetBuffer[0] = MassConfigRequest_ID; 
    memcpy(packetBuffer, requestPacket, sizeof(MassConfigRequestPacket));
    Serial.write(packetBuffer, sizeof(MassConfigRequestPacket));
}

void Cosco::sendMassConfigResponsePacket(MassConfigResponsePacket *responsePacket)
{
    // Serialize and send MassConfigResponsePacket
    uint8_t packetBuffer[sizeof(MassConfigResponsePacket) + 1];
    packetBuffer[0] = MassConfigResponse_ID;
    memcpy(packetBuffer, responsePacket, sizeof(MassConfigResponsePacket));
    Serial.write(packetBuffer, sizeof(MassConfigResponsePacket));
}

void Cosco::sendMassDataPacket(MassData *responsePacket)
{
    // Serialize and send sendMassDataPacket
    uint8_t packetBuffer[sizeof(MassData) + 1];
    packetBuffer[0] = MassData_ID;
    memcpy(packetBuffer, responsePacket, sizeof(MassData));
    Serial.write(packetBuffer, sizeof(MassData));
}

void Cosco::receive(MassConfigPacket *configPacket, MassConfigRequestPacket *requestPacket, MassConfigResponsePacket *responsePacket)
{
    // Check if data is available
    if (Serial.available() > 0) {
        // Read the incoming data into a buffer
        char buffer[64];
        int len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[len] = '\0'; // Null-terminate the string

        // Read and store the first byte (the "first bit")
        uint8_t firstByte = static_cast<uint8_t>(buffer[0]);
        // Do something with the stored value, e.g., print it
        Serial.print("First byte received: 0x");
        Serial.println(firstByte, HEX);

    }
}

void Cosco::init_LEDS(){
    initialize_strip();

    pinMode(LED_PIN, OUTPUT);
    strip->begin();

    //init timer 
    // Initialize Timer (Timer 0, Divider 80 -> 1 tick = 1�s)
    timer = timerBegin(0, 80, true);  // Timer 0, prescaler 80 (1�s tick), count up

    // Attach ISR
    timerAttachInterrupt(timer, &onTimer, true); // Edge-triggered

    // Set timer alarm (1000000 �s = 1 second)
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
