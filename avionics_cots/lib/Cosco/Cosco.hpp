/**
 * @file Cosco.hpp
 * @author Eliot Abramo
*/
#ifndef COSCO_HPP
#define COSCO_HPP

#include "packet_definition.hpp"
#include "ADS1234.hpp"

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
    void sendMassDataPacket(MassData *responsePacket);

    /**
     * @brief functions that receive commands  
     * 
     * @param configPacket 
     * @param requestPacket 
     * @param responsePacket 
     */    
    void receive(MassConfigPacket* configPacket, MassConfigRequestPacket* requestPacket, MassConfigResponsePacket* responsePacket);


    /**
     * @brief Initialize leds 
     * 
     * @return null 
     */
    void init_LEDS();

     /**
     * @brief recieve led info
     * 
     * @return null 
     */
    void recieve_LED_packet();

     /**
     * @brief update LED
     * 
     * @return null 
     */
    void update_LEDS();
};

#endif /* COSCO_HPP */