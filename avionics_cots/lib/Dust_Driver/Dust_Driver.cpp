/**
 * @file Servo.cpp
 * @author Ilyas Asmouki
*/
#include "Dust_Driver.hpp"

Dust::Dust() :  pm1_0_(0), pm2_5_(0), pm10__(0), num_particles_0_3_(0), num_particles_0_5_(0), num_particles_1_0_(0),
                num_particles_2_5_(0), num_particles_5_0_(0), num_particles_10__(0) {
                
    sensor = new HM330X();
}

Dust::~Dust() {
    delete sensor;
    sensor = nullptr;
}

void Dust::init() {
    dust_monitor.log("Initializing Dust Sensor");
    if (sensor->init(I2C_DUST_SDA, I2C_DUST_SCL)) {
        alive = false;
        dust_monitor.log("Dust Sensor init failed");
        return;
    }
    alive = true;

    dust_monitor.log("Dust Sensor Initialized.");
}
void Dust::loop() {
    if (sensor->read_sensor_value(buf, 29)) {
        dust_monitor.log("Dust Sensor read failed");
        return;
    }

    parse_sensor_data(buf);
    // for (size_t i = 0; i < 30; i++) dust_monitor.log(String(buf[i]) + " ");
    // dust_monitor.log();


    dust_monitor.log("PM1.0_STD:" + String(pm1_0_) + ", PM2.5_STD:" + String(pm2_5_) + ", PM10_STD:" + String(pm10__));
    dust_monitor.log(", NUM_PTC_0_3:" + String(num_particles_0_3_) + ", NUM_PTC_0_5:" + String(num_particles_0_5_) +
                        ", NUM_PTC_1_0:" + String(num_particles_1_0_) + ", NUM_PTC_2_5:" + String(num_particles_2_5_) +
                        ", NUM_PTC_5_0:" + String(num_particles_5_0_) + ", NUM_PTC_10 :" + String(num_particles_10__) + "\n");

    DustData packet = {
        .pm1_0 = pm1_0_,
        .pm2_5 = pm2_5_,
        .pm10_ = pm10__,
        .num_particles_0_3 = num_particles_0_3_,
        .num_particles_0_5 = num_particles_0_5_,
        .num_particles_1_0 = num_particles_1_0_,
        .num_particles_2_5 = num_particles_2_5_,
        .num_particles_5_0 = num_particles_5_0_,
        .num_particles_10_ = num_particles_10__,
    };
    
    dust_handler.sendDustDataPacket(&packet);
    delay(1000 / SAMPLING_RATE);
}

void Dust::parse_sensor_data(uint8_t* data) {
    if (data != NULL) {
        pm2_5_ = (uint16_t)(data[PM2_5_STD] << sizeof(data)) + data[PM2_5_STD+1];
        pm1_0_ = (uint16_t)(data[PM1_0_STD] << sizeof(data)) + data[PM1_0_STD+1];
        pm10__ = (uint16_t)(data[PM10__STD] << sizeof(data)) + data[PM10__STD+1];

        num_particles_0_3_ = (uint16_t)(data[NUM_PARTICLES_0_3] << sizeof(data)) + data[NUM_PARTICLES_0_3+1];
        num_particles_0_5_ = (uint16_t)(data[NUM_PARTICLES_0_5] << sizeof(data)) + data[NUM_PARTICLES_0_5+1];
        num_particles_1_0_ = (uint16_t)(data[NUM_PARTICLES_1_0] << sizeof(data)) + data[NUM_PARTICLES_1_0+1];
        num_particles_2_5_ = (uint16_t)(data[NUM_PARTICLES_2_5] << sizeof(data)) + data[NUM_PARTICLES_2_5+1];
        num_particles_5_0_ = (uint16_t)(data[NUM_PARTICLES_5_0] << sizeof(data)) + data[NUM_PARTICLES_5_0+1];
        num_particles_10__ = (uint16_t)(data[NUM_PARTICLES_10_] << sizeof(data)) + data[NUM_PARTICLES_10_+1];
    }
}

bool Dust::is_alive() {
    return alive;
}