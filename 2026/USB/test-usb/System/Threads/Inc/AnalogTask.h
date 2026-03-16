#ifndef THREADS_INC_ANALOGTASK_H_
#define THREADS_INC_ANALOGTASK_H_

#include "MessageThread.h"
#include "adc.h" // To get access to hadc1 or hadc3

class AnalogTask : public MessageThread<EmptyMessage, AnalogPacket> {
public:
    AnalogTask();
    void init() override;
    void loop() override;

private:
    float _voltage = 0.0f;
    int _currentHat = -1;    // The stable ID currently sent to ROS
    int _candidateHat = 0;   // The ID we are currently "watching"
    int _confirmCounter = 0; // Consecutive samples of the candidate
};

#endif /* THREADS_INC_ANALOGTASK_H_ */
