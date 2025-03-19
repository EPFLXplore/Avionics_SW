/**
 * @file Servo.hpp
 * @author Eliot Abramo
*/
#ifndef DUST_SENSOR_HPP
#define DUST_SENSOR_HPP

#include <Seeed_HM330X.h>
#include <Arduino.h>
#include "driver/ledc.h"

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    #define SERIAL_OUTPUT SerialUSB
#else
    #define SERIAL_OUTPUT Serial
#endif

#define I2C_DUST_SDA    21
#define I2C_DUST_SCL    22

#define BAUDRATE 115200

#define SENS_BUF_SIZE 30
#define SAMPLING_RATE 1 // Hz

//num of PMx sized particles in ug/m^3
#define PM1_0_STD         5-1
#define PM2_5_STD         7-1
#define PM10__STD         9-1

//num of particles below given size in um in 1 litre of air 
#define NUM_PARTICLES_0_3 17-1
#define NUM_PARTICLES_0_5 19-1
#define NUM_PARTICLES_1_0 21-1
#define NUM_PARTICLES_2_5 23-1
#define NUM_PARTICLES_5_0 25-1
#define NUM_PARTICLES_10_ 27-1

class Dust
{
public:
    Dust();
    ~Dust();

    void init();
    void loop();

    bool is_alive();

private:
    HM330X* sensor = nullptr;
    uint8_t buf[SENS_BUF_SIZE] = {0};  // sensor data buffer

    void parse_sensor_data(uint8_t* data);

    uint16_t pm1_0 = 0;
    uint16_t pm2_5 = 0;
    uint16_t pm10_ = 0;

    uint16_t num_particles_0_3 = 0;
    uint16_t num_particles_0_5 = 0;
    uint16_t num_particles_1_0 = 0;
    uint16_t num_particles_2_5 = 0;
    uint16_t num_particles_5_0 = 0;
    uint16_t num_particles_10_ = 0;

    bool alive = false;
};

#endif /** DUST_SENSOR_HPP */