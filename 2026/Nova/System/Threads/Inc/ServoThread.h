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
	DRILL,

	// Sentinel id: one ServoRequest drives BOTH service modules atomically.
	// Publish a single message with id = SERVICE_MODULE_BOTH instead of two.
	SERVICE_MODULE_BOTH = 10
};

class ServoThread : public MessageThread<ServoRequest, EmptyMessage> {
public:
    ServoThread(const char* name, osPriority priority);
    virtual ~ServoThread();

    void init() override;
    void loop() override;

private:
    PWMDriver*   servo[4];

    // Auto-stop for continuous-rotation servos (LEFT/RIGHT_SERVICE_MODULE)
    TickType_t   _last_cmd_tick[4] = {};
    bool         _stop_pending[4]  = {};
};
