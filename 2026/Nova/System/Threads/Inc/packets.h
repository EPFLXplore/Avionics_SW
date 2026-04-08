/*
 * packets.h
 *
 *  Created on: Nov 29, 2025
 *      Author: pedro
 */

#ifndef THREADS_INC_PACKETS_H_
#define THREADS_INC_PACKETS_H_


struct BeatPacket {
	int32_t beat;
};


struct TestPacket {
	int32_t ping;
};

struct ServoRequest {
    uint8_t id;
    int32_t increment;
    bool zero_in;
    uint32_t status_code;
};

struct MassPacket {
    uint8_t id;
    float mass;
};

struct MassRequest{
    uint8_t id; //TODO: add board id and slave id to all cmd
    bool tare;
};

struct LedRequest {
    uint8_t id;
    uint8_t system;
    uint8_t mode;
};






#endif /* THREADS_INC_PACKETS_H_ */
