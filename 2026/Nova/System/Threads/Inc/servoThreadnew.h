#pragma once
#include "MessageThread.h"
#include "packets.h"
#include "ServoConfigs.h"

class ServoThread : public MessageThread<ServoRequest, EmptyMessage> {
public:
    ServoThread(const char* name, osPriority priority);
    virtual ~ServoThread();

    void init() override;
    void loop() override;

private:
    PWMDriver* servo[4];
};
