#pragma once
#include "MessageThread.h"
#include "packets.h"
#include "ServoConfigs.h"

enum Servos_ID {
	FRONT_CAM,
	LEFT_SERVICE_MODULE,
	RIGHT_SERVICE_MODULE,
	LEFT_LED,
	RIGHT_LED,
	DRILL
};

class ServoThread : public MessageThread<ServoRequest, EmptyMessage> {
public:
    ServoThread(const char* name, osPriority priority);
    virtual ~ServoThread();

    void init() override;
    void loop() override;

private:
    PWMDriver* servo[4];
};
