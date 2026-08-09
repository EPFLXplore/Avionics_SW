#pragma once
#include "MessageThread.h"
#include "packets.h"
#include "device_ids.h"    // ServoId: the shared, fleet-wide device ids
#include "BoardProfile.h"  // which ServoId sits on which SPI pin, per board
#include "ServoConfigs.h"

// ServoChannel / SERVO_CHANNEL_COUNT live in BoardProfile.h, beside the table
// they index. Only the wire speaks ServoId; everything here speaks channels.
//
// The LED strips are not servos: they go through LedsThread over LEDRequest and
// have neither a channel nor a ServoId.

class ServoThread : public MessageThread<ServoRequest, EmptyMessage> {
public:
    ServoThread(const char* name, osPriority priority);
    virtual ~ServoThread();

    /** True when this board carries at least one servo. Read live off the
     *  profile at the moment System asks, NOT latched at construction: that
     *  keeps the start decision where HEAD had it - in System::init, at start
     *  time - instead of freezing it into a bool several statements earlier.
     *  System asks this instead of reading the profile itself, so nothing above
     *  the threads knows what a channel is. */
    bool hasDevices() const { return anyDevice(profile().servo); }

    void init() override;
    void loop() override;

private:

    // Plain objects. Safe because ServoThread is constructed at runtime (from
    // System::init, after HAL), so PWMDriver's HAL timer setup runs post-HAL.
    //
    // A channel is constructed live only when the profile declares a device on
    // it; otherwise it is inert and touches neither pin nor timer. That is what
    // keeps TIM15 clear for the WS2812 strip on a board that declares no servos,
    // and stops boards with none from configuring timers they never use.
    PWMDriver servo[SERVO_CHANNEL_COUNT] = {
        PWMDriver(SERVO_0_CFG, profile().servo[0] != NO_DEVICE),
        PWMDriver(SERVO_1_CFG, profile().servo[1] != NO_DEVICE),
        PWMDriver(SERVO_2_CFG, profile().servo[2] != NO_DEVICE),
        PWMDriver(SERVO_3_CFG, profile().servo[3] != NO_DEVICE),
    };
};
