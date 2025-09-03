/**
 * @file main.cpp
 * @author Eliot Abramo
 */

#include <Arduino.h>
#include <Seeed_HM330X.h>
#include <GyverHX711.h>
#include "soc/rtc.h"
#include "packet_definition.hpp"
#include "Nexus.hpp"
#include "Servo.hpp"
#include "Dust_Driver.hpp"

/**
 * servo id 1 = cam front
 * servo id 3 = drill clapet 
 * mass id 5 = mass drill
 * mass id 7 = mass hd
*/

// Pin definition
#define HD_DOUT 35
#define HD_SCK   32
#define DRILL_DOUT    25
#define DRILL_SCK     26

#define SERVO_DRILL_PIN 16
#define SERVO_DRILL_CHAN 1
#define SERVO_CAM_PIN   17
#define SERVO_CAM_CHAN  0


// Type definitions
Nexus nexus;
Servo_Driver* servo_cam = new Servo_Driver();
Servo_Driver* servo_drill = new Servo_Driver();
Dust* dust = new Dust();

constexpr uint8_t AVG_SIZE = 10;

GyverHX711 mass_drill(DRILL_DOUT, DRILL_SCK, HX_GAIN64_A);
GyverHX711 mass_hd   (HD_DOUT,    HD_SCK,   HX_GAIN64_A);

/******************************* Mass Code  *******************************************/
/**
 * CHANGE THIS PLEASE
 * I didn't have time to segment this into pointers and a different class so it lives here for now.
 * But please clean this up and create a class.
 * 
 */
float drillBuf[AVG_SIZE] = {0};
float hdBuf[AVG_SIZE]    = {0};

float offset_drill = 0.0f;
float offset_hd    = 0.0f;

float slope_drill  = 0.01028f;   // g per count
float slope_hd     = 0.01028f;

float weight_drill = 0.0f;
float weight_hd    = 0.0f;

void shift(float *array , int N, float valueIn){  //shifts all array values left and adds valueIn at position N-1
  for(int i = 1; i<N-1 ; i++){
    array[i-1] = array[i];
  }
  array[N-1] = valueIn;
}

float movingAverage(const float *arr, uint8_t n) {
  if(n<=0){return 0;}
  float sum = 0.0f;
  for (uint8_t i = 0; i < n; i++) sum += arr[i];
  return sum / n;
}

void updateDrill() {
    if (!mass_drill.available()) return;
    long raw = mass_drill.read();
    shift(drillBuf, AVG_SIZE, (float) raw);
    float avg = movingAverage(drillBuf, AVG_SIZE);
    weight_drill = (avg - offset_drill) * slope_drill;
}

void updateHD() {
    if (!mass_hd.available()) return;
    long raw = mass_hd.read();
    shift(hdBuf, AVG_SIZE, (float) raw);
    float avg = movingAverage(hdBuf, AVG_SIZE);
    weight_hd = (avg - offset_hd) * slope_hd;
}

void tareScales() {
    offset_drill = mass_drill.read();
    offset_hd    = mass_hd.read();
    mass_drill.tare();
    mass_hd.tare();
    for (uint8_t i = 0; i < AVG_SIZE; ++i) {
        drillBuf[i] = offset_drill;
        hdBuf[i]    = offset_hd;
    }
    delay(100);
}
/*******************************************************************************************/


void setup() {
  // lower CPU clock to 80 MHz (saves power, reduces noise)
  rtc_cpu_freq_config_t cfg;
  rtc_clk_cpu_freq_get_config(&cfg);
  rtc_clk_cpu_freq_to_config(RTC_CPU_FREQ_80M, &cfg);
  rtc_clk_cpu_freq_set_config_fast(&cfg);

  Serial.begin(115200);

  // Mass
  mass_drill.tare();  
  offset_drill = mass_drill.read();

  mass_hd.tare();  
  offset_hd = mass_hd.read();

  // Servo
  servo_cam->init(SERVO_CAM_PIN, SERVO_CAM_CHAN);
  servo_drill->init(SERVO_DRILL_PIN, SERVO_DRILL_CHAN);

  // Dust
  dust->init();
}

void loop() {
  /**
   * Receive command from Nexus and determine if change is needed 
   * Change is only for the Mass. Part of the things THAT YOU NEED TO CHANGE PLEASE.
   * This just allows for a very very simple fsm and direct access.
   * Time was the issue here, so please take the time to think of something better :)
   */
  Change changeMass = nexus.receive(servo_cam, servo_drill);

  switch (changeMass.id) {

    case MassDrill_Request_ID:
    {
      offset_drill = mass_drill.read();
      mass_drill.tare();
      for (uint8_t i = 0; i < AVG_SIZE; ++i) {
          drillBuf[i] = offset_drill;
      }

      MassPacket drill_change = {
        MassDrill_ID,
        weight_drill
      };

      nexus.sendMassPacket(&drill_change, MassDrill_ID);
      delay(100);
      break;
    }

    case MassHD_Request_ID:
    {
      offset_hd    = mass_hd.read();
      mass_hd.tare();
      for (uint8_t i = 0; i < AVG_SIZE; ++i) {
          hdBuf[i]    = offset_hd;
      }
      delay(100);

      MassPacket hd_change = {
        MassHD_ID,
        weight_drill
      };

      nexus.sendMassPacket(&hd_change, MassDrill_ID);

      break;
    }
  }

  updateDrill();
  updateHD();

  static uint32_t lastMass = 0;    // ms
  static uint32_t last_send_dust = 0;

  if (millis() - lastMass >= 1000) {
    lastMass = millis();
    MassPacket drill = {
      MassDrill_ID,
      weight_drill
    };
    nexus.sendMassPacket(&drill, MassDrill_ID);

    MassPacket hd = {
      MassHD_ID,
      weight_hd
    };
    nexus.sendMassPacket(&hd, MassHD_ID);

    // If mass above 200g for the drill, then we just put the Servo back under rover.
    // Removed as not deemed useful for competition.
    // if(weight_drill >= 200){
    //   ServoRequest request = {
    //     ServoDrill_ID,
    //     -1000,
    //     false
    //   };
    //   servo_drill->set_request(request);
      // servo_drill->handle_servo();   
    // }
    // Serial.printf("Drill: %.2f g | HD: %.2f g\n", drill.mass, hd.mass);
  }

  if (millis() - last_send_dust >= 1000) {
    last_send_dust = millis();
    if(dust->is_alive()){
      DustData dust_packet;
      dust->loop(&dust_packet);
      nexus.sendDustDataPacket(&dust_packet);
    }
  }

  nexus.sendHeartbeat();

}

