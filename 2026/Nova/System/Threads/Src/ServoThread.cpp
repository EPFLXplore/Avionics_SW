#include <ServoThread.h>

ServoThread::ServoThread(const char* name, osPriority priority)
    : MessageThread(name, priority)
{
    // DeviceType ids are NOT bound here: see init().
}

ServoThread::~ServoThread()
{
    // _servo[] are plain member objects: nothing to free.
}

void ServoThread::init()
{
    // Bind local channels to global device ids.
    // This is the ONLY place the board profile is bound; everything else works
    // off _servo[].globalId.
    bindSlots<SlotGroup::Pwm>(_servo, DeviceType::Servo);

    // _servo[] are already constructed (member objects, built when this thread
    // was constructed in System::init, after HAL), so their timers run - but the
    // CCRs are still at CubeMX's Pulse = 0, which is no pulse at all. A servo
    // with no pulse is unpowered: it holds nothing and is free to drift. That is
    // safe HERE specifically - nothing on these channels carries enough load to
    // backdrive a servo and damage it, so an unpowered boot costs a little drift
    // and nothing else. On a mechanism that could fall under its own weight this
    // would be the wrong default.
    //
    // ...except the lamp switch, which is not a servo and has to be taken back
    // off the timer. Its PWMDriver already claimed the pad as an alternate
    // function and started the channel when this thread was constructed;
    // configPin() here overrides that to a push-pull output, and runs LATER
    // because init() is task context. The timer keeps running on a channel
    // whose pad no longer listens to it, which costs nothing.
    //
    // Driven LOW first: lamps off at boot, before anything commands them.
    for (uint8_t i = 0; i < PWM_COUNT; ++i) {
        if (_servo[i].globalId != idOf(ServoIdType::LampPower)) continue;
        _lampPad  = pwmPinOf(static_cast<ConnType>(PWM_FIRST + i));
        _hasLamp  = true;
        configPin(_lampPad, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
        writePin(_lampPad, GPIO_PIN_RESET);
        break;
    }

    // Homing here would mean driving to SERVO_ZERO_DEG, the compiled FALLBACK,
    // before servo_cal.yaml has been replayed - so every boot would slam the
    // hardware to a position the RPi is about to correct. Nothing moves until
    // something asks: the first ServoRequest, or a go_to_zero once Nexus has
    // sent the real home angles on link-up.
}

void ServoThread::loop()
{
    // Every channel drives a POSITIONAL _servo: the angle written to the CCR is
    // a commanded position, held until the next command.

    // Block for a command, then drain and execute the whole queue. Commands for
    // devices this board does not carry are popped and dropped.
    ServoRequest req;
    if (this->waitCommand(req, pdMS_TO_TICKS(10))) {
        do {
            this->apply(req, req.id, false);

            // Coupling servos to ensure service module opening doesnt break
            if (!req.change_zero) {
                for (const ServoCouple& c : SERVO_COUPLES) {
                    if (req.id == c.a) this->apply(req, c.b, c.mirrored);
                    if (req.id == c.b) this->apply(req, c.a, c.mirrored);
                }
            }
        } while (this->popCommand(req));
    }
}

void ServoThread::apply(const ServoRequest& req, uint8_t id, bool mirrored)
{
    // The lamp switch, before anything servo-shaped happens. A PWM channel cannot
    // express off: angleToPulseUs() clamps into 500..2500 us of a 20 ms frame, so
    // the quietest a servo pulse gets is 2.5% duty - dim lamps flickering at
    // 50 Hz, not darkness. A GPIO level can, so this one is switched, not driven.
    //
    // Anything above 0 is ON; go_to_zero and angle 0 are OFF. change_zero means
    // nothing here - there is no home position to trim.
    if (id == idOf(ServoIdType::LampPower)) {
        if (!_hasLamp || req.change_zero) return;
        const bool on = !req.go_to_zero && req.angle > 0;
        writePin(_lampPad, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
        return;
    }

    PWMDriver* s = deviceFor(_servo, id);
    if (!s) return;   // not carried by this board: drop it, as every request always has

    if (req.change_zero) {
        s->setZeroPulseUs(angleToPulseUs((float)req.zero));
    } else if (req.go_to_zero) {
        // Its OWN home, mirrored or not: the mirror is a property of the
        // commanded angle, and a configured home is already stated per servo.
        s->zero();
    } else {
        const float angle = (float)req.angle;
        s->setAngle(mirrored ? ANGLE_MAX_DEG - angle : angle);
    }
}
