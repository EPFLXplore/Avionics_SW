/*
 * LEDStrip.cpp
 *
 *  Created on: Mar 11, 2026
 *      Author: Mohamed Gdoura (Original by Eliot)
 */

#include "LEDStrip.h"

/** Milliseconds since boot, as the pattern engines measure time. */
static inline unsigned long nowMs() { return xTaskGetTickCount(); }

LEDStrip::LEDStrip(uint8_t numLeds)
    : _numLeds(numLeds), _strip(numLeds) {}

void LEDStrip::begin(TIM_HandleTypeDef *timer, uint32_t channel, bool complementary) {
    _stripTimer = timer;
    _stripChannel = channel;
	_strip.begin(_stripTimer, _stripChannel, complementary);
    _strip.setBrightness(70);
    _strip.show();

    uint8_t segmentSize = 100 / MAX_SYSTEMS;

    for (uint8_t i = 0; i < MAX_SYSTEMS; ++i) {
        _cmds[i].mode = static_cast<uint8_t>(LedModeType::Off);
        _cmds[i].segment.low  = i * segmentSize;
        _cmds[i].segment.high = (i + 1) * segmentSize - 1;
        _cmds[i].segment.r = _cmds[i].segment.g = _cmds[i].segment.b = 0;
        _states[i] = {};  // zero initialize
    }
    _strip.show();
}

void LEDStrip::setBrightness(uint8_t b) { _strip.setBrightness(b); }

// convert percentage (0-100) to pixel index
int LEDStrip::pctToIdx(uint8_t pct) const
{
	int idx = (pct * _numLeds) / 100;
	if (idx >= _numLeds) idx = _numLeds - 1;
	return idx;
}

void LEDStrip::applyCommand(const Command& cmd) {
	if (cmd.system >= MAX_SYSTEMS)
		return;
	_cmds[cmd.system] = cmd;
	_states[cmd.system] = { };                            // reset state
}

void LEDStrip::tick() {
    for (uint8_t i = 0; i < MAX_SYSTEMS; ++i)
    	handleMode(i);
    _strip.show();
}

void LEDStrip::tickOneSystem(uint8_t idx)
{
	handleMode(idx);
	_strip.show();
}

void LEDStrip::setAll(int start, int end, uint8_t r, uint8_t g, uint8_t b) {
	if (end >= _numLeds) end = _numLeds - 1;
    for (int i = start; i <= end; ++i) _strip.setPixelColor(i, {r, g, b}, true);
//    _strip.show();
}

void LEDStrip::handleMode(uint8_t idx) {
    const Command& cmd = _cmds[idx];
    int start = pctToIdx(cmd.segment.low);
    int end   = pctToIdx(cmd.segment.high);
    switch (static_cast<LedModeType>(cmd.mode)) {
        case LedModeType::Off:   modeOff(idx, start, end); break;
        case LedModeType::On:    modeOn(idx, start, end, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::Blink: modeBlink(idx, start, end, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::Fault: modeFault(idx, start, end, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::EmergencyMotors:   modeEmergencyMotors(idx, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::EmergencyShutdown: modeEmergencyShutdown(idx, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::AllOff: modeAllOff(idx); break;
    }

}

/******************  MODE IMPLEMENTATIONS  ******************/
void LEDStrip::modeOff(uint8_t idx, int s, int e) {
    if (!_states[idx].initialized) {
        setAll(s, e, 0, 0, 255);
        _states[idx].initialized = true;
    }
}

void LEDStrip::modeOn(uint8_t idx, int s, int e, uint8_t r, uint8_t g, uint8_t b) {
    if (!_states[idx].initialized) {
        setAll(s, e, r, g, b);
        _states[idx].initialized = true;
    }
}

void LEDStrip::modeBlink(uint8_t idx, int s, int e, uint8_t r, uint8_t g, uint8_t b,
                     uint8_t eye, uint16_t speed, uint16_t pause) {
//    ModeState& st = _states[idx];
//    if (HAL_GetTick() - st.lastUpdate < speed) return;
//
//    // Calculate current head position
//    int head = s + st.step;
//    // Clear previous frame
//    setAll(s, e, 0, 0, 0);
//
//    // Draw eye
//    _strip.setPixelColor(head, {r / 10, g / 10, b / 10}, true);
//    for (uint8_t j = 1; j <= eye; ++j)
//        _strip.setPixelColor(head + j, {r, g, b}, true);
//    _strip.setPixelColor(head + eye + 1, {r / 10, g / 10, b / 10}, true);
////    _strip.show();
//
//    st.lastUpdate = HAL_GetTick();
//    st.step += st.phase == 0 ? 1 : -1;
//
//    // Bounce at ends
//    if (head >= e - eye - 2) { st.phase = 1; st.lastUpdate += pause; }
//    else if (head <= s)      { st.phase = 0; st.lastUpdate += pause; }

	 ModeState &st = _states[idx];
	if (HAL_GetTick() - st.lastUpdate < speed)
		return;

	int head = s + st.step;
	setAll(s, e, 0, 0, 0);

	// bounds-checked drawing
	auto safeSet = [&](int px, uint8_t r, uint8_t g, uint8_t b) {
		if (px >= s && px <= e)
			_strip.setPixelColor(px, { r, g, b }, true);
	};

	safeSet(head, r / 10, g / 10, b / 10);
	for (uint8_t j = 1; j <= eye; ++j)
		safeSet(head + j, r, g, b);
	safeSet(head + eye + 1, r / 10, g / 10, b / 10);

	st.lastUpdate = HAL_GetTick();
	st.step += st.phase == 0 ? 1 : -1;

	if (head >= e - (int) eye - 2) {
		st.phase = 1;
		st.lastUpdate += pause;
	} else if (head <= s) {
		st.phase = 0;
		st.lastUpdate += pause;
	}
}

void LEDStrip::modeFault(uint8_t idx, int s, int e, uint8_t r, uint8_t g, uint8_t b, uint16_t speed) {
	ModeState &st = _states[idx];
	if (HAL_GetTick() - st.lastUpdate < speed)
		return;

	if (st.phase == 0) {
		// Fade on
		setAll(s, e, r, g, b);
		st.phase = 1;
	} else {
		// Fade off
		setAll(s, e, 0, 0, 0);
		st.phase = 0;
	}

	st.lastUpdate = HAL_GetTick();
//	if (HAL_GetTick() - st.lastUpdate < speed)
//		return;
//
//	for (int i = s + st.phase; i < e; i += 3)
//		_strip.setPixelColor(i, { r, g, b }, true);
//
//	// clear old
//	for (int i = s + st.phase; i < e; i += 3)
//		_strip.setPixelColor(i, { 0, 0, 0 }, true);
//
//	_strip.show();
//
//	st.phase = (st.phase + 1) % 3;
//	st.lastUpdate = HAL_GetTick();
//    ModeState& st = _states[idx];
//    if (HAL_GetTick() - st.lastUpdate < speed) return;
//
//    int prevPhase = (st.phase == 0) ? 2 : (st.phase - 1);
//    for (int i = s + st.phase; i < e; i += 3)
//        _strip.setPixelColor(i, {0, 0, 0}, true);
//
//    // clear old
//    for (int i = s + st.phase; i < e; i += 3)
//        _strip.setPixelColor(i, {r, g, b}, true);
//
//    st.phase = (st.phase + 1) % 3;
//    st.lastUpdate = HAL_GetTick();
}

void LEDStrip::modeEmergencyMotors(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (!_states[idx].initialized) {
        setAll(0, 100, r, g, b);
        _states[idx].initialized = true;
    }
}

void LEDStrip::modeEmergencyShutdown(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (!_states[idx].initialized) {
        setAll(0, 100, r, g, b);
        _states[idx].initialized = true;
    }
}

void LEDStrip::modeAllOff(uint8_t idx) {
    if (!_states[idx].initialized) {
        setAll(0, 100, 0, 0, 255);
        _states[idx].initialized = true;
    }
}


