/**
 * @file Dust_Driver.hpp
 * @author Eliot Abramo, Ilyas Asmouki
*/
#ifndef DUST_SENSOR_HPP
#define DUST_SENSOR_HPP

#include <Seeed_HM330X.h>
#include <Arduino.h>
#include "driver/ledc.h"
#include "packet_definition.hpp"
// #include "monitor.hpp"

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    #define SERIAL_OUTPUT SerialUSB
#else
    #define SERIAL_OUTPUT Serial
#endif

#define I2C_DUST_SDA    21
#define I2C_DUST_SCL    22

#define BAUDRATE 115200

#define SENS_BUF_SIZE 29
#define SAMPLING_RATE 1 // Hz

//num of PMx sized particles in ug/m^3
#define PM1_0_STD           5-1     // 2 bytes
#define PM2_5_STD           7-1     // 2 bytes
#define PM10__STD           9-1     // 2 bytes
#define PM1_0_ATM           11-1    // 2 bytes
#define PM2_5_ATM           13-1    // 2 bytes
#define PM10__ATM           15-1    // 2 bytes

//num of particles below given size in um in 1 litre of air 
#define NUM_PARTICLES_0_3   17-1    // 2 bytes
#define NUM_PARTICLES_0_5   19-1    // 2 bytes
#define NUM_PARTICLES_1_0   21-1    // 2 bytes
#define NUM_PARTICLES_2_5   23-1    // 2 bytes
#define NUM_PARTICLES_5_0   25-1    // 2 bytes
#define NUM_PARTICLES_10_   27-1    // 2 bytes

// end of buffer
#define CHECKSUM            29-1    // 2 bytes
#define BUFSIZE             29

class Dust
{
public:
    /**
     * @brief Construct a new Dust object
     */
    Dust();

    /**
     * @brief Destroy the Dust object
     */
    ~Dust();

    /**
     * @brief Initializes the Dust Sensor
     * @return null
     */
    void init();

    /**
     * @brief Loop function that reads dust sensor data periodically
     * @return null
     */
    void loop(DustData *dustData);

    /**
     * @brief Asks the Dust Instance if the sensor is working
     * @return alive boolean
     */
    bool is_alive();

private:
    HM330X* sensor = nullptr;
    uint8_t buf[BUFSIZE] = {0};  // sensor data buffer

    /**
     * @brief Parses incoming data
     * @param data
     * @return null
     */
    void parse_sensor_data(uint8_t* data);

    uint16_t pm1_0_std_ = 0;
    uint16_t pm2_5_std_ = 0;
    uint16_t pm10__std_ = 0;

    uint16_t pm1_0_atm_ = 0;
    uint16_t pm2_5_atm_ = 0;
    uint16_t pm10__atm_ = 0;

    uint16_t num_particles_0_3_ = 0;
    uint16_t num_particles_0_5_ = 0;
    uint16_t num_particles_1_0_ = 0;
    uint16_t num_particles_2_5_ = 0;
    uint16_t num_particles_5_0_ = 0;
    uint16_t num_particles_10__ = 0;

    bool alive = false;

    unsigned long lastSampleTime_;
};

#endif /** DUST_SENSOR_HPP */