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
};
struct MassPacket {
    uint8_t id;
    float mass;
};

struct AnalogPacket {
    float voltage;
    int hat_id; // 0: None, 1: HAT1, 2: HAT2
};






#endif /* THREADS_INC_PACKETS_H_ */
