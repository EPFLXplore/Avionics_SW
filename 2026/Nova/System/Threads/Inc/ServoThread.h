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

    // Auto-stop for continuous-rotation servos (LEFT/RIGHT_SERVICE_MODULE):
    // 1 s after the last command the servo is driven to 90° (neutral/stop).
    TickType_t _last_cmd_tick[4] = {};
    bool       _stop_pending[4]  = {};
};
