/*
 * packets.h
 *
 *  Created on: Nov 29, 2025
 *      Author: pedro
 */

#ifndef THREADS_INC_PACKETS_H_
#define THREADS_INC_PACKETS_H_


enum class PacketType : uint8_t {
    SERVO_REQUEST,
    HEARTBEAT,
    MASS_PACKET
};

struct Heartbeat {
    uint8_t beat;
};

struct ServoRequest {
    uint8_t id;
    int32_t increment;
    bool zero_in;
};
struct MassPacket {
    uint8_t id;
    float mass;
};

struct SystemMessage {
    PacketType type;
    union {
        ServoRequest    servo;
        Heartbeat       heartbeat;
        MassPacket      mass_packet;
    } data;
};





#endif /* THREADS_INC_PACKETS_H_ */
