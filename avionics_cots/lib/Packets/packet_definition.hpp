/** 
 * @file packet_definition.hpp 
 * @author Eliot Abramo 
*/ 

#ifndef PACKET_DEFINITION_H
#define PACKET_DEFINITION_H

#include <iostream>
#include <packet_id.hpp>

struct AngleArray {
    uint16_t id;
    float angles[4];
};

struct FourInOne {
    uint16_t id;
    float temperature;
    float moisture;
    float conductivity;
    float ph;
};

struct LEDMessage {
    uint8_t low;
    uint8_t high;
    uint8_t system;
    uint8_t mode;
};

struct LEDResponse {
    bool success;
};

struct MassArray {
    uint16_t id;
    float mass[4];
};

struct MassCalibOffset {
    uint16_t destination_id;
    uint8_t channel;
};

struct MassCalibScale {
    uint16_t destination_id;
    uint8_t channel;
    float expected_weight;
};

struct MassConfigPacket {
    uint16_t destination_id;
    float offset[4];
    float scale[4];
    float alpha;
    bool enabled_channels[4];
    bool remote_command;
    bool set_offset;
    bool set_scale;
    bool set_alpha;
    bool set_channels_status;
};

struct MassConfigRequestPacket {
    uint16_t id;
    bool req_offset;
    bool req_scale;
    bool req_alpha;
    bool req_channels_status;
};

struct MassConfigResponsePacket {
    uint16_t id;
    float offset[4];
    float scale[4];
    float alpha;
    bool enabled_channels[4];
    bool remote_command;
    bool set_offset;
    bool set_scale;
    bool set_alpha;
    bool set_channels_status;
    bool success;
};

struct NPK {
    uint16_t id;
    uint16_t nitrogen;
    uint16_t phosphorus;
    uint16_t potassium;
};

struct ServoRequest {
    uint16_t id;
    float angle;
    bool zero_in;
};

struct ServoResponse {
    uint16_t id;
    float angle;
    bool success;
};

#endif /* PACKET_DEFINITION_H */