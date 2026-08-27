/*
 * LEDStrip.cpp
 *
 *  Created on: Mar 11, 2026
 *      Author: Mohamed Gdoura (Original by Eliot)
 */

#include "LEDStrip.h"

LEDStrip::LEDStrip(uint8_t numLeds)
    : _numLeds(numLeds), _strip(numLeds) {}

void LEDStrip::begin(TIM_HandleTypeDef *timer, uint32_t channel, bool complementary) {
	_strip.begin(timer, channel, complementary);
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


// convert percentage (0-100) to pixel index
int LEDStrip::pctToIdx(uint8_t percent) const
{
	int index = (percent * _numLeds) / 100;
	if (index >= _numLeds) index = _numLeds - 1;
	return index;
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

void LEDStrip::setAll(int start, int end, uint8_t r, uint8_t g, uint8_t b) {
	if (end >= _numLeds) end = _numLeds - 1;
    for (int i = start; i <= end; ++i) _strip.setPixelColor(i, {r, g, b});
//    _strip.show();
}

void LEDStrip::handleMode(uint8_t system) {
    const Command& cmd = _cmds[system];
    int start = pctToIdx(cmd.segment.low);
    int end   = pctToIdx(cmd.segment.high);
    switch (static_cast<LedModeType>(cmd.mode)) {
        case LedModeType::Off:   modeOff(system, start, end); break;
        case LedModeType::On:    modeOn(system, start, end, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::Blink: modeBlink(system, start, end, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::Fault: modeFault(system, start, end, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::EmergencyMotors:   modeEmergencyMotors(system, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::EmergencyShutdown: modeEmergencyShutdown(system, cmd.segment.r, cmd.segment.g, cmd.segment.b); break;
        case LedModeType::AllOff: modeAllOff(system); break;
    }

}

/******************  MODE IMPLEMENTATIONS  ******************/
/* Off is dark. It used to paint a resting blue so a quiet segment still proved
 * the board was alive, but that made "subsystem not running" and "showing blue
 * on purpose" the same picture; an operator could not tell them apart. */
void LEDStrip::modeOff(uint8_t system, int start, int end) {
    if (!_states[system].initialized) {
        setAll(start, end, 0, 0, 0);
        _states[system].initialized = true;
    }
}

void LEDStrip::modeOn(uint8_t system, int start, int end, uint8_t r, uint8_t g, uint8_t b) {
    if (!_states[system].initialized) {
        setAll(start, end, r, g, b);
        _states[system].initialized = true;
    }
}

/* Bounds-checked pixel write: the blink head walks past both ends of the
 * segment by design, so every draw has to be clipped. */
void LEDStrip::setBounded(int px, int start, int end, const Color& color) {
	if (px >= start && px <= end)
		_strip.setPixelColor(px, color);
}

void LEDStrip::modeBlink(uint8_t system, int start, int end, uint8_t r, uint8_t g, uint8_t b,
                     uint8_t eye, uint16_t speed, uint16_t pause) {
	 ModeState &state = _states[system];
	if (HAL_GetTick() - state.lastUpdate < speed)
		return;

	int head = start + state.step;
	setAll(start, end, 0, 0, 0);

	setBounded(head, start, end, { uint8_t(r / 10), uint8_t(g / 10), uint8_t(b / 10) });
	for (uint8_t j = 1; j <= eye; ++j)
		setBounded(head + j, start, end, { r, g, b });
	setBounded(head + eye + 1, start, end, { uint8_t(r / 10), uint8_t(g / 10), uint8_t(b / 10) });

	state.lastUpdate = HAL_GetTick();
	state.step += state.phase == 0 ? 1 : -1;

	if (head >= end - (int) eye - 2) {
		state.phase = 1;
		state.lastUpdate += pause;
	} else if (head <= start) {
		state.phase = 0;
		state.lastUpdate += pause;
	}
}

void LEDStrip::modeFault(uint8_t system, int start, int end, uint8_t r, uint8_t g, uint8_t b, uint16_t speed) {
	ModeState &state = _states[system];
	if (HAL_GetTick() - state.lastUpdate < speed)
		return;

	if (state.phase == 0) {
		// Fade on
		setAll(start, end, r, g, b);
		state.phase = 1;
	} else {
		// Fade off
		setAll(start, end, 0, 0, 0);
		state.phase = 0;
	}

	state.lastUpdate = HAL_GetTick();
}

void LEDStrip::modeEmergencyMotors(uint8_t system, uint8_t r, uint8_t g, uint8_t b) {
    if (!_states[system].initialized) {
        setAll(0, 100, r, g, b);
        _states[system].initialized = true;
    }
}

void LEDStrip::modeEmergencyShutdown(uint8_t system, uint8_t r, uint8_t g, uint8_t b) {
    if (!_states[system].initialized) {
        setAll(0, 100, r, g, b);
        _states[system].initialized = true;
    }
}

void LEDStrip::modeAllOff(uint8_t system) {
    if (!_states[system].initialized) {
        setAll(0, 100, 0, 0, 0);
        _states[system].initialized = true;
    }
}


