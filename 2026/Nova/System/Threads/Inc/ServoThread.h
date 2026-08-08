#pragma once
#include "MessageThread.h"
#include "packets.h"
#include "ServoConfigs.h"

/**
 * A servo id IS the index into servo[] below, i.e. the physical channel: id N
 * drives SERVO_N_CFG. Renumbering these therefore moves wiring, not just the
 * value carried in a ServoRequest - see ServoConfigs.h for the pin per channel.
 *
 * Only ids 0..3 exist as outputs (four PWM channels). LEFT_LED / RIGHT_LED sit
 * past that on purpose: the strips are driven by LedsThread over LEDRequest,
 * not as servos, and a ServoRequest naming them is ignored by loop().
 */
enum Servos_ID {
	FRONT_CAM            = 0,  // PB15 TIM15_CH2
	DRILL                = 1,  // PB14 TIM15_CH1
	LEFT_SERVICE_MODULE  = 2,  // PB13 TIM1_CH1N (complementary)
	RIGHT_SERVICE_MODULE = 3,  // PB11 TIM2_CH4

	LEFT_LED             = 4,  // not a servo channel (see above)
	RIGHT_LED            = 5,  // not a servo channel (see above)

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
    // servoTimerFree() constructs a servo inert when its timer is owned by
    // another subsystem on this board role (TIM15 -> LED strip on id 3).
    PWMDriver servo[4] = {
        PWMDriver(SERVO_0_CFG, servoTimerFree(SERVO_0_CFG)),
        PWMDriver(SERVO_1_CFG, servoTimerFree(SERVO_1_CFG)),
        PWMDriver(SERVO_2_CFG, servoTimerFree(SERVO_2_CFG)),
        PWMDriver(SERVO_3_CFG, servoTimerFree(SERVO_3_CFG)),
    };

    // Last SERVICE_MODULE_BOTH open/close command: 0=open, 180=close, -1=none
    int16_t      _last_both_cmd    = -1;
};
