/**
 * @file Nexus.hpp
 * @author Eliot Abramo
 * @brief Direct bus communication for ESP32-
 *  This file is the “glue” in the middle.                                                │
*/

#ifndef NEXUS_HPP
#define NEXUS_HPP

#include "packet_definition.hpp"
#include "packet_id.hpp"
#include <functional>    // For std::function
#include <unordered_map> // For std::unordered_map
#include "Servo.hpp"

/**
 * @brief Change struct helps handle the Mass sensor tare requests from the CS.
 * Library default constructors don't handle pointers well and makes stack panic.
 */
struct Change {
  uint8_t id;
  bool tare;
  float scale;
};

class Nexus {
public:
    /**
     * @brief Create a new Nexus Object
     */
    Nexus();
    
    /**
     * @brief Destroys a Nexus Object. Should unalocate any pointers and memory used up in class
     */    
    ~Nexus();
    
    /**
     * @brief Send mass data  packet
     * 
     * @param configPacket: pointer to packet to be sent. Defined in Packets->->packet_definition.hpp
     * @return null 
     */
    void sendMassPacket(MassPacket *responsePacket, uint8_t ID);

    /**
     * @brief Send sensor data packet
     * 
     * @param dataPacket: pointer to packet to be sent. Defined in Packets->->packet_definition.hpp
     * @return null 
     */
    void sendDustDataPacket(DustData *dataPacket);

    /**
     * @brief functions that receive commands  
     * 
     * @param configPacket 
     * @param requestPacket 
     * @param responsePacket 
     * @return null
     */    
    Change receive(Servo_Driver* servo_cam, Servo_Driver* servo_drill);

    /**
     * @brief Send heartbeat packet
     * @return null
     */
    void sendHeartbeat();

};

#endif /* Nexus_HPP */