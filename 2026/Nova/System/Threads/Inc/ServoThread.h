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
    // Plain objects. Safe because ServoThread is constructed at runtime (from
    // System::init, after HAL), so PWMDriver's HAL timer setup runs post-HAL.
    PWMDriver servo[4] = {
        PWMDriver(SERVO_0_CFG),
        PWMDriver(SERVO_1_CFG),
        PWMDriver(SERVO_2_CFG),
        PWMDriver(SERVO_3_CFG),
    };

    // Auto-stop for continuous-rotation servos (LEFT/RIGHT_SERVICE_MODULE)
    TickType_t   _last_cmd_tick[4] = {};
    bool         _stop_pending[4]  = {};

    // Last SERVICE_MODULE_BOTH open/close command: 0=open, 180=close, -1=none
    int16_t      _last_both_cmd    = -1;
};
