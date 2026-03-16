#include "AnalogTask.h"
#include <cmath> // for std::fabs

// Link to the ADC handle created by CubeMX in adc.c
extern ADC_HandleTypeDef hadc1;

// Configuration Constants
#define HAT1_VOLTAGE    1.0f
#define HAT2_VOLTAGE    2.0f
#define HAT_TOLERANCE   0.1f
#define DEBOUNCE_SAMPLES 5

AnalogTask::AnalogTask()
: MessageThread("AnalogTask")
{
    // 50ms delay = 20Hz sampling rate
    setTickDelay(50);
}

void AnalogTask::init() {
    // Calibrate the ADC on startup for the H7
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
}

void AnalogTask::loop() {
    // 1. Hardware Read
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t raw = HAL_ADC_GetValue(&hadc1);
        // 12-bit conversion (0 to 4095)
        _voltage = (float)raw * (3.3f / 4095.0f);
    }
    HAL_ADC_Stop(&hadc1);

    // 2. Identify the "Instant" detection
    int detectedNow = 0; // Default: Disconnected
    if (std::fabs(_voltage - HAT1_VOLTAGE) < HAT_TOLERANCE) {
        detectedNow = 1;
    } else if (std::fabs(_voltage - HAT2_VOLTAGE) < HAT_TOLERANCE) {
        detectedNow = 2;
    }

    // 3. Debounce State Machine
    if (detectedNow == _candidateHat) {
        // We see the same potential HAT as the last loop
        _confirmCounter++;
    } else {
        // The voltage changed! Reset the confirmation process
        _candidateHat = detectedNow;
        _confirmCounter = 0;
    }

    // 4. Update and Push to ROS only if stable and changed
    // We require 'DEBOUNCE_SAMPLES' consecutive identical readings
    if (_confirmCounter >= DEBOUNCE_SAMPLES && _candidateHat != _currentHat) {
        _currentHat = _candidateHat;

        AnalogPacket pkt;
        pkt.voltage = _voltage;
        pkt.hat_id = _currentHat;

        // This sends the data to the MicroRosThread queue
        pushStatus(pkt);
    }
}
