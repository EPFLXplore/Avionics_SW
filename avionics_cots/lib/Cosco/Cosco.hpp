/**
 * @file Cosco.hpp
 * @author Eliot Abramo
*/
#ifndef COSCO_HPP
#define COSCO_HPP

#include "packet_definition.hpp"
#include "packet_id.hpp"
#include "HX711.h"
#include <functional>    // For std::function
#include <unordered_map> // For std::unordered_map



class Cosco {
public:
    /**
     * @brief Create a new Cosco Object
     */
    Cosco();

    /**
     * @brief Destroys a Cosco Object. Should unalocate any pointers and memory used up in class
     */
    ~Cosco();

    /**
     * @brief Send mass configuration packet
     *
     * @param configPacket: pointer to packet to be sent. Defined in Packets->packet_definition.hpp
     * @return null
     */
    void sendMassConfigPacket(MassConfigPacket* configPacket);

    /**
     * @brief Send mass configuration packet
     *
     * @param configPacket: pointer to packet to be sent. Defined in Packets->->packet_definition.hpp
     * @return null
     */
    void sendMassConfigRequestPacket(MassConfigRequestPacket* requestPacket);

    /**
     * @brief Send mass configuration response packet
     *
     * @param configPacket: pointer to packet to be sent. Defined in Packets->->packet_definition.hpp
     * @return null
     */
    void sendMassConfigResponsePacket(MassConfigResponsePacket* responsePacket);

    /**
     * @brief Send mass data  packet
     *
     * @param configPacket: pointer to packet to be sent. Defined in Packets->->packet_definition.hpp
     * @return null
     */
<<<<<<< HEAD
    void sendMassDataPacket(MassData* responsePacket);


    /**
     * @brief Send mass configuration packet
     *
     * @param requestPacket: pointer to packet to be sent. Defined in Packets->->packet_definition.hpp
     * @return null
     */
    void sendServoRequestPacket(ServoRequest* requestPacket);

    /**
     * @brief Send mass configuration response packet
     *
     * @param responsePacket: pointer to packet to be sent. Defined in Packets->->packet_definition.hpp
     * @return null
     */
    void sendServoResponsePacket(ServoResponse* responsePacket);

    /**
     * @brief Send sensor data packet
     *
     * @param dataPacket: pointer to packet to be sent. Defined in Packets->->packet_definition.hpp
     * @return null
     */
    void sendDustDataPacket(DustData* dataPacket);


    /**
     * @brief functions that receive commands
     *
     * @param configPacket
     * @param requestPacket
     * @param responsePacket
     * @return null
     */
    void receive(void* packet);
=======
    void sendMassDataPacket(MassData *responsePacket);

    /**
     * @brief functions that receive commands  
     * 
     * @param configPacket 
     * @param requestPacket 
     * @param responsePacket 
     */    
    void receive(MassConfigPacket* configPacket, MassConfigRequestPacket* requestPacket, MassConfigResponsePacket* responsePacket);
>>>>>>> 3a6b2f0be72ce4afffa8409f90482291cc3b5c7e
};

#endif /* COSCO_HPP */