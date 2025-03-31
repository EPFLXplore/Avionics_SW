/**
 * @file Cosco.cpp
 * @author Eliot Abramo
*/
#include "Cosco.hpp"
#include <Wire.h>
#include <Arduino.h>
#include <packet_id.hpp>
#include <packet_definition.hpp>

#include "ADS1234.hpp"
#include "packet_definition.hpp"
#include "packet_id.hpp"

Cosco::Cosco()
{
    Serial.begin(115200);
    Serial.println("Serial launched");
}

Cosco::~Cosco() {}

void Cosco::sendMassConfigPacket(MassConfigPacket* configPacket)
{
    // Serialize and send MassConfigPacket
    uint8_t packetBuffer[sizeof(MassConfigPacket) + 1];
    packetBuffer[0] = MassConfig_ID;
    memcpy(packetBuffer + 1, configPacket, sizeof(MassConfigPacket));
    Serial.write(packetBuffer, sizeof(MassConfigPacket));
}

void Cosco::sendMassConfigRequestPacket(MassConfigRequestPacket* requestPacket)
{
    // Serialize and send MassConfigRequestPacket
    uint8_t packetBuffer[sizeof(MassConfigRequestPacket) + 1];
    packetBuffer[0] = MassConfigRequest_ID;
    memcpy(packetBuffer, requestPacket, sizeof(MassConfigRequestPacket));
    Serial.write(packetBuffer, sizeof(MassConfigRequestPacket));
}

void Cosco::sendMassConfigResponsePacket(MassConfigResponsePacket* responsePacket)
{
    // Serialize and send MassConfigResponsePacket
    uint8_t packetBuffer[sizeof(MassConfigResponsePacket) + 1];
    packetBuffer[0] = MassConfigResponse_ID;
    memcpy(packetBuffer, responsePacket, sizeof(MassConfigResponsePacket));
    Serial.write(packetBuffer, sizeof(MassConfigResponsePacket));
}

void Cosco::sendMassDataPacket(MassData* responsePacket)
{
    // Serialize and send sendMassDataPacket
    uint8_t packetBuffer[sizeof(MassData) + 1];
    packetBuffer[0] = MassData_ID;
    memcpy(packetBuffer, responsePacket, sizeof(MassData));
    Serial.write(packetBuffer, sizeof(MassData));
}

void Cosco::receive(void* packet)
{
    uint8_t packet_id = 0; // Variable to hold the packet ID
    // Check if data is available
    if (Serial.available() > 0) {
        // Read the incoming data into a buffer
        char buffer[128] = { 0 }; // Buffer to hold incoming data
        int len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        buffer[len] = '\0'; // Null-terminate the string

        // Read and store the first byte (the "first bit")
        packet_id = static_cast<uint8_t>(buffer[0]);

        switch (packet_id) {
        case MassData_ID: HANDLE_PACKET(MassPacket);
        case MassConfigRequest_ID: HANDLE_PACKET(MassConfigRequestPacket);
            // case MassCalib_ID: HANDLE_PACKET();
            // case MassConfig_ID: HANDLE_PACKET();
        case MassConfigResponse_ID: HANDLE_PACKET(MassConfigResponsePacket);

        default: {
            Serial.println("Unknown packet ID");
            break;
        }
        }
    }
<<<<<<< HEAD
}
=======
}

>>>>>>> 3a6b2f0be72ce4afffa8409f90482291cc3b5c7e
