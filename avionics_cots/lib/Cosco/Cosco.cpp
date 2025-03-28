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
    memcpy(packetBuffer + 1, requestPacket, sizeof(MassConfigRequestPacket));
    Serial.write(packetBuffer, sizeof(MassConfigRequestPacket));
}

void Cosco::sendMassConfigResponsePacket(MassConfigResponsePacket *responsePacket)
{
    // Serialize and send MassConfigResponsePacket
    uint8_t packetBuffer[sizeof(MassConfigResponsePacket) + 1];
    packetBuffer[0] = MassConfigResponse_ID;
    memcpy(packetBuffer + 1, responsePacket, sizeof(MassConfigResponsePacket));
    Serial.write(packetBuffer, sizeof(MassConfigResponsePacket));
}

void Cosco::sendMassDataPacket(MassData *responsePacket)
{
    // Serialize and send sendMassDataPacket
    uint8_t packetBuffer[sizeof(MassData) + 1];
    packetBuffer[0] = MassData_ID;
    memcpy(packetBuffer + 1, responsePacket, sizeof(MassData));
    Serial.write(packetBuffer, sizeof(MassData));
}

void Cosco::sendDustDataPacket(DustData *dataPacket)
{
    // Serialize and send sendDustDataPacket
    uint8_t packetBuffer[sizeof(DustData) + 1];
    packetBuffer[0] = DustData_ID;
    memcpy(packetBuffer + 1, dataPacket, sizeof(DustData));
    Serial.write(packetBuffer, sizeof(DustData));
}

#define HANDLE_PACKET(packet_type) do {                                         \
    if (len == sizeof(packet_type)) {                                           \
        memcpy(packet, buffer + 1, sizeof(packet_type));                        \
        Serial.println(String(#packet_type) + " packet copied successfully");   \
    } else {                                                                    \
        Serial.println("Received data too "                                     \
                        + String(len > sizeof(packet_type) ? "long" : "short")  \
                        + " for " + String(#packet_type));                      \
    }                                                                           \
    break;                                                                      \
} while (0)

void sendServoRequestPacket(ServoRequest* requestPacket)
{
    // Serialize and send sendServoRequestPacket
    uint8_t packetBuffer[sizeof(ServoRequest) + 1];
    packetBuffer[0] = ServoConfigRequest_ID;
    memcpy(packetBuffer + 1, requestPacket, sizeof(ServoRequest));
    Serial.write(packetBuffer, sizeof(ServoRequest));
}

void sendServoResponsePacket(ServoResponse* responsePacket)
{
    // Serialize and send sendServoResponsePacket
    uint8_t packetBuffer[sizeof(ServoResponse) + 1];
    packetBuffer[0] = ServoResponse_ID;
    memcpy(packetBuffer + 1, responsePacket, sizeof(ServoResponse));
    Serial.write(packetBuffer, sizeof(ServoResponse));
}

void Cosco::receive(MassConfigPacket *configPacket, MassConfigRequestPacket *requestPacket, MassConfigResponsePacket *responsePacket)
{
    // Check if data is available
    if (Serial.available() > 0) {
        // Read the incoming data into a buffer
        char buffer[128] = {0};
        int len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[len] = '\0'; // Null-terminate the string

        // Read and store the first byte (the "first bit")
        uint8_t packet_id = static_cast<uint8_t>(buffer[0]);
        // Do something with the stored value, e.g., print it
        Serial.print("First byte received: 0x");
        Serial.println(packet_id, HEX);

        switch (packet_id) {
            case MassData_ID: HANDLE_PACKET(MassArray);
                break;
            case MassConfigRequest_ID: HANDLE_PACKET(MassConfigRequestPacket);
                break;
            // case MassCalib_ID: HANDLE_PACKET();
            // case MassConfig_ID: HANDLE_PACKET();
            case MassConfigResponse_ID: HANDLE_PACKET(MassConfigResponsePacket);
            // case Servo_ID: HANDLE_PACKET();
            case ServoResponse_ID: HANDLE_PACKET(ServoResponse);
            // case ServoConfigRequest_ID: HANDLE_PACKET();
            // case ServoConfig_ID: HANDLE_PACKET();
            // case ServoConfigResponse_ID: HANDLE_PACKET(ServoConfigResponse);
            case LED_ID: HANDLE_PACKET(LEDMessage);
            case LEDResponse_ID: HANDLE_PACKET(LEDResponse);
            case FourInOne_ID: HANDLE_PACKET(FourInOne);
            case NPK_ID: HANDLE_PACKET(NPK);
            case DustData_ID: HANDLE_PACKET(DustData);
            default: {
                Serial.println("Unknown packet ID");
                break;
            }
        }
    }
}

#undef HANDLE_PACKET

