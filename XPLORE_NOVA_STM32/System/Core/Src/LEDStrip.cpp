/*
 * LEDStrip.cpp
 *
 *  Created on: Mar 11, 2026
 *      Author: Mohamed Gdoura (Original by Eliot)
 */

#include "LEDStrip.h"
#include "cmsis_os2.h"   // osKernelGetTickCount()

// NOW_MS() stops incrementing once FreeRTOS takes SysTick.
// Use the RTOS tick counter instead (1 tick = 1 ms at configTICK_RATE_HZ=1000).
#define NOW_MS() osKernelGetTickCount()

LEDStrip::LEDStrip(uint8_t numLeds)
    : _numLeds(numLeds), _strip(numLeds) {}

void LEDStrip::begin(TIM_HandleTypeDef *timer, uint32_t channel) {
    stripTimer = timer;
    stripChannel = channel;
	_strip.begin(stripTimer, stripChannel);
    _strip.setBrightness(70);
    _strip.show();

    uint8_t segmentSize = 100 / MAX_SYSTEMS;

    for (uint8_t i = 0; i < MAX_SYSTEMS; ++i) {
        _cmds[i].mode = 0;   // default off
        _cmds[i].segment.low  = i * 33;
        _cmds[i].segment.high = (i + 1) * 33 - 1;
        _cmds[i].segment.r = _cmds[i].segment.g = _cmds[i].segment.b = 0;
        _states[i] = {};  // zero initialize
    }
}

void LEDStrip::setBrightness(uint8_t b) { _strip.setBrightness(b); }

// convert percentage (0-100) to pixel index
int LEDStrip::pctToIdx(uint8_t pct) const
{
	return (pct * _numLeds) / 100;
}

void LEDStrip::applyCommand(const Command& cmd) {
    if (cmd.system >= MAX_SYSTEMS) return;
    // Only reset animation state when the mode changes.
    // Repeated calls with the same mode (e.g. every loop tick) must not
    // zero lastUpdate/step, or animated modes 2/3 can never advance.
    if (_cmds[cmd.system].mode != cmd.mode) {
        _states[cmd.system] = {};
    }
    _cmds[cmd.system] = cmd;
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
    for (int i = start; i <= end; ++i) _strip.setPixelColor(i, {r, g, b}, true);
    // No show() here — callers (tick / tickOneSystem) call show() once per frame.
}

void LEDStrip::handleMode(uint8_t idx) {
    const Command& cmd = _cmds[idx];
    int start = pctToIdx(cmd.segment.low);
    int end   = pctToIdx(cmd.segment.high);
    switch (cmd.mode) {
        case 0: mode0(idx, start, end); break; // OFF
        case 1: mode1(idx, start, end, cmd.segment.r, cmd.segment.g, cmd.segment.b); break; // ON
        case 2: mode2(idx, start, end, cmd.segment.r, cmd.segment.g, cmd.segment.b); break; // BLINK
        case 3: mode3(idx, start, end, cmd.segment.r, cmd.segment.g, cmd.segment.b); break; // FAULT
        case 4: mode4(idx, cmd.segment.r, cmd.segment.g, cmd.segment.b); break; // EMERGENCY_MOTORS
        case 5: mode5(idx, cmd.segment.r, cmd.segment.g, cmd.segment.b); break; // EMERGENCY_GLOBAL
        case 6: mode6(idx); break; // ALL_OFF
        default: break;
    }

}

/******************  MODE IMPLEMENTATIONS  ******************/
void LEDStrip::mode0(uint8_t idx, int s, int e) {
    if (!_states[idx].initialized) {
        setAll(s, e, 0, 0, 255);
        _states[idx].initialized = true;
    }
}

void LEDStrip::mode1(uint8_t idx, int s, int e, uint8_t r, uint8_t g, uint8_t b) {
    if (!_states[idx].initialized) {
        setAll(s, e, r, g, b);
        _states[idx].initialized = true;
    }
}

void LEDStrip::mode2(uint8_t idx, int s, int e, uint8_t r, uint8_t g, uint8_t b,
                     uint8_t eye, uint16_t speed, uint16_t pause) {
    ModeState& st = _states[idx];
    if (NOW_MS() - st.lastUpdate < speed) return;

    int head = s + st.step;
    setAll(s, e, 0, 0, 0);  // clear segment

    // Draw eye with bounds check so we never write outside [s, e]
    uint8_t dimR = (uint8_t)(r / 10), dimG = (uint8_t)(g / 10), dimB = (uint8_t)(b / 10);
    if (head >= s && head <= e)
        _strip.setPixelColor(head, {dimR, dimG, dimB}, true);
    for (uint8_t j = 1; j <= eye; ++j)
        if (head + j <= e) _strip.setPixelColor(head + j, {r, g, b}, true);
    if (head + eye + 1 <= e)
        _strip.setPixelColor(head + eye + 1, {dimR, dimG, dimB}, true);
    // show() is called by tick() — no inline show here

    st.lastUpdate = NOW_MS();
    st.step += st.phase == 0 ? 1 : -1;

    if (head >= e - (int)eye - 2) { st.phase = 1; st.lastUpdate += pause; }
    else if (head <= s)           { st.phase = 0; st.lastUpdate += pause; }
}

void LEDStrip::mode3(uint8_t idx, int s, int e, uint8_t r, uint8_t g, uint8_t b, uint16_t speed) {
    ModeState& st = _states[idx];
    if (NOW_MS() - st.lastUpdate < speed) return;

    // Clear the segment first, then draw this phase's pixels.
    // tick() calls show() after all modes run — no inline show here.
    setAll(s, e, 0, 0, 0);
    for (int i = s + st.phase; i <= e; i += 3)
        _strip.setPixelColor(i, {r, g, b}, true);

    st.phase = (st.phase + 1) % 3;
    st.lastUpdate = NOW_MS();
}

void LEDStrip::mode4(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (!_states[idx].initialized) {
        setAll(0, _numLeds - 1, r, g, b);
        _states[idx].initialized = true;
    }
}

void LEDStrip::mode5(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (!_states[idx].initialized) {
        setAll(0, _numLeds - 1, r, g, b);
        _states[idx].initialized = true;
    }
}

void LEDStrip::mode6(uint8_t idx) {
    if (!_states[idx].initialized) {
        setAll(0, _numLeds - 1, 0, 0, 0);
        _states[idx].initialized = true;
    }
}


