/**
 * @file Servo.cpp
 * @author Ilyas Asmouki
*/
#include "Dust_Driver.hpp"

Dust::Dust() :  pm1_0(0), pm2_5(0), pm10_(0), num_particles_0_3(0), num_particles_0_5(0), num_particles_1_0(0),
                num_particles_2_5(0), num_particles_5_0(0), num_particles_10_(0) {
                
    sensor = new HM330X();
}

Dust::~Dust() {
    delete sensor;
}

void Dust::init() {
    SERIAL_OUTPUT.println("Initializing Dust Sensor");
    if (sensor->init(I2C_DUST_SDA, I2C_DUST_SCL)) {
        alive = false;
        SERIAL_OUTPUT.println("Dust Sensor init failed");
        return;
    }
    alive = true;

    SERIAL_OUTPUT.println("Dust Sensor Initialized.");
}
void Dust::loop() {
    if (sensor->read_sensor_value(buf, 29)) {
        SERIAL_OUTPUT.println("Dust Sensor read failed");
        return;
    }

    parse_sensor_data(buf);
    // for (size_t i = 0; i < 30; i++) SERIAL_OUTPUT.print(String(buf[i]) + " ");
    // SERIAL_OUTPUT.println();


    SERIAL_OUTPUT.print("PM1.0_STD:" + String(pm1_0) + ", PM2.5_STD:" + String(pm2_5) + ", PM10_STD:" + String(pm10_));
    SERIAL_OUTPUT.print(", NUM_PTC_0_3:" + String(num_particles_0_3) + ", NUM_PTC_0_5:" + String(num_particles_0_5) +
                        ", NUM_PTC_1_0:" + String(num_particles_1_0) + ", NUM_PTC_2_5:" + String(num_particles_2_5) +
                        ", NUM_PTC_5_0:" + String(num_particles_5_0) + ", NUM_PTC_10 :" + String(num_particles_10_) + "\n");

    delay(1000 / SAMPLING_RATE);
}

void Dust::parse_sensor_data(uint8_t* data) {
    if (data != NULL) {
        pm2_5 = (uint16_t)(data[PM2_5_STD] << sizeof(data)) + data[PM2_5_STD+1];
        pm1_0 = (uint16_t)(data[PM1_0_STD] << sizeof(data)) + data[PM1_0_STD+1];
        pm10_ = (uint16_t)(data[PM10__STD] << sizeof(data)) + data[PM10__STD+1];

        num_particles_0_3 = (uint16_t)(data[NUM_PARTICLES_0_3] << sizeof(data)) + data[NUM_PARTICLES_0_3+1];
        num_particles_0_5 = (uint16_t)(data[NUM_PARTICLES_0_5] << sizeof(data)) + data[NUM_PARTICLES_0_5+1];
        num_particles_1_0 = (uint16_t)(data[NUM_PARTICLES_1_0] << sizeof(data)) + data[NUM_PARTICLES_1_0+1];
        num_particles_2_5 = (uint16_t)(data[NUM_PARTICLES_2_5] << sizeof(data)) + data[NUM_PARTICLES_2_5+1];
        num_particles_5_0 = (uint16_t)(data[NUM_PARTICLES_5_0] << sizeof(data)) + data[NUM_PARTICLES_5_0+1];
        num_particles_10_ = (uint16_t)(data[NUM_PARTICLES_10_] << sizeof(data)) + data[NUM_PARTICLES_10_+1];
    }
}

bool Dust::is_alive() {
    return alive;
}