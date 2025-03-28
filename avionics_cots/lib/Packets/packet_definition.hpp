/** 
 * @file packet_definition.hpp 
 * @author Eliot Abramo 
*/ 

#ifndef PACKET_DEFINITION_H
#define PACKET_DEFINITION_H

#include <iostream>
#include <packet_id.hpp>

struct LEDMessage {
    uint8_t low;
    uint8_t high;
    uint8_t system;
    uint8_t mode;
};

struct DustData {
    uint16_t pm1_0_std;;
    uint16_t pm2_5_std;;
    uint16_t pm10__std;;
    uint16_t pm1_0_atm;;
    uint16_t pm2_5_atm;;
    uint16_t pm10__atm;;
    uint16_t num_particles_0_3;;
    uint16_t num_particles_0_5;;
    uint16_t num_particles_1_0;;
    uint16_t num_particles_2_5;;
    uint16_t num_particles_5_0;;
    uint16_t num_particles_10_;;
};

struct FourInOne {
    uint16_t id;
    float temperature;
    float moisture;
    float conductivity;
    float ph;
};

struct ServoResponse {
    uint16_t id;
    float angle;
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

struct BMS {
    String status;
    float v_bat;
    float current;
    uint32_t voltages[4];
};

struct MassCalibScale {
    uint16_t destination_id;
    uint8_t channel;
    float expected_weight;
};

struct ServoRequest {
    uint16_t id;
    uint8_t increment;
    bool zero_in;
};

struct ServoResponse {
    uint16_t id;
    float angle;
    bool success;
};

#endif /* PACKET_DEFINITION_H */