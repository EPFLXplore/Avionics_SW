/**
 * @file Servo.cpp
 * @author Ilyas Asmouki
*/
#include "Dust_Driver.hpp"

Dust::Dust() :  pm1_0_std_(0), pm2_5_std_(0), pm10__std_(0),
                pm1_0_atm_(0), pm2_5_atm_(0), pm10__atm_(0),
                num_particles_0_3_(0), num_particles_0_5_(0), num_particles_1_0_(0),
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

    dust_monitor.log("Dust Sensor Initialized");
}
void Dust::loop() {
    if (sensor->read_sensor_value(buf, BUFSIZE)) {
        dust_monitor.log("Dust Sensor read failed");
        return;
    }

    parse_sensor_data(buf);

    dust_monitor.log("PM1.0_STD:" + String(pm1_0_std_) + ", PM2.5_STD:" + String(pm2_5_std_) + ", PM10_STD:" + String(pm10__std_) + "\n");
    dust_monitor.log("PM1.0_ATM:" + String(pm1_0_atm_) + ", PM2.5_ATM:" + String(pm2_5_atm_) + ", PM10_ATM:" + String(pm10__atm_) + "\n");
    dust_monitor.log("NUM_PTC_0_3:" + String(num_particles_0_3_) + ", NUM_PTC_0_5:" + String(num_particles_0_5_) +
                        ", NUM_PTC_1_0:" + String(num_particles_1_0_) + ", NUM_PTC_2_5:" + String(num_particles_2_5_) +
                        ", NUM_PTC_5_0:" + String(num_particles_5_0_) + ", NUM_PTC_10 :" + String(num_particles_10__) + "\n");

    DustData packet = {
        .pm1_0_std = pm1_0_std_,
        .pm2_5_std = pm2_5_std_,
        .pm10__std = pm10__std_,
        .pm1_0_atm = pm1_0_atm_,
        .pm2_5_atm = pm2_5_atm_,
        .pm10__atm = pm10__atm_,
        .num_particles_0_3 = num_particles_0_3_,
        .num_particles_0_5 = num_particles_0_5_,
        .num_particles_1_0 = num_particles_1_0_,
        .num_particles_2_5 = num_particles_2_5_,
        .num_particles_5_0 = num_particles_5_0_,
        .num_particles_10_ = num_particles_10__,
    };

    dust_monitor.log("BEGIN SEND DATA\n");
    
    dust_handler.sendDustDataPacket(&packet);

    dust_monitor.log("END SEND DATA\n");

    DustData received_packet = {0};

    dust_handler.receive(&received_packet);
    dust_monitor.log("RECEIVED: " + String(received_packet.pm1_0_std) + " " + String(received_packet.pm2_5_std) + " " + String(received_packet.pm10__std) + "\n");
    dust_monitor.log("RECEIVED: " + String(received_packet.pm1_0_atm) + " " + String(received_packet.pm2_5_atm) + " " + String(received_packet.pm10__atm) + "\n");
    dust_monitor.log("RECEIVED: " + String(received_packet.num_particles_0_3) + " " + String(received_packet.num_particles_0_5) + " " + String(received_packet.num_particles_1_0) + "\n");
    dust_monitor.log("RECEIVED: " + String(received_packet.num_particles_2_5) + " " + String(received_packet.num_particles_5_0) + " " + String(received_packet.num_particles_10_) + "\n");

    delay(1000 / SAMPLING_RATE);
}

void Dust::parse_sensor_data(uint8_t* data) {
    uint16_t checksum = 0;

    dust_monitor.log("SIZE OF DATA = " + String(sizeof(*data)));

    if (data != NULL) {
        pm2_5_std_ = ((uint16_t)(data[PM2_5_STD] << 8) + data[PM2_5_STD+1]);
        pm1_0_std_ = ((uint16_t)(data[PM1_0_STD] << 8) + data[PM1_0_STD+1]);
        pm10__std_ = ((uint16_t)(data[PM10__STD] << 8) + data[PM10__STD+1]);

        pm2_5_atm_ = ((uint16_t)(data[PM2_5_ATM] << 8) + data[PM2_5_ATM+1]);
        pm1_0_atm_ = ((uint16_t)(data[PM1_0_ATM] << 8) + data[PM1_0_ATM+1]);
        pm10__atm_ = ((uint16_t)(data[PM10__ATM] << 8) + data[PM10__ATM+1]);

        num_particles_0_3_ = ((uint16_t)(data[NUM_PARTICLES_0_3] << 8) + data[NUM_PARTICLES_0_3+1]);
        num_particles_0_5_ = ((uint16_t)(data[NUM_PARTICLES_0_5] << 8) + data[NUM_PARTICLES_0_5+1]);
        num_particles_1_0_ = ((uint16_t)(data[NUM_PARTICLES_1_0] << 8) + data[NUM_PARTICLES_1_0+1]);
        num_particles_2_5_ = ((uint16_t)(data[NUM_PARTICLES_2_5] << 8) + data[NUM_PARTICLES_2_5+1]);
        num_particles_5_0_ = ((uint16_t)(data[NUM_PARTICLES_5_0] << 8) + data[NUM_PARTICLES_5_0+1]);
        num_particles_10__ = ((uint16_t)(data[NUM_PARTICLES_10_] << 8) + data[NUM_PARTICLES_10_+1]);

        checksum = data[CHECKSUM];
    }

    uint8_t calculated_checksum = 0;
    for (size_t i = 0; i < CHECKSUM; ++i) {
        calculated_checksum += data[i];
    }
    if (calculated_checksum == checksum) {
        dust_monitor.log("CHECKSUM SUCCESS " + String(calculated_checksum) + "\n");
    } else {
        dust_monitor.log("CHECKSUM FAILED, CALCULATED: " + String(calculated_checksum) + " ACTUAL: " + String(checksum) + "\n");
    }
}

bool Dust::is_alive() {
    return alive;
}