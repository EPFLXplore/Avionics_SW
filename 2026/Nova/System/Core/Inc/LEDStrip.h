/*
 * LEDStrip.h
 *
 *  Created on: Mar 10, 2026
 *      Author: Mohamed Gdoura  (Original by Eliot)
 */

#pragma once

#include "WS2812Driver.h"
#include "device_ids.h"   // LedSystemType, LedModeType: the shared wire vocabulary

struct Segment {
    uint8_t low;   // percentage 0-100
    uint8_t high;  // percentage 0-100
    uint8_t r, g, b;
};

struct Command {
    Segment segment;
    uint8_t emergency_global; // 0 or 1
    uint8_t emergency_motors; // 0 or 1
    uint8_t system;  // 0-2
    uint8_t mode;    // 0-6
};

struct ModeState {
    unsigned long lastUpdate = 0;
    int step   = 0;
    int phase  = 0;
    bool initialized = false;
};

/**
 * One segment of the strip per rover subsystem.
 *
 * DERIVED from LedSystemType rather than stated again: _cmds[] and _states[] are
 * indexed by cmd.system, so a subsystem added to the enum without growing this
 * would write past the end of both arrays (applyCommand only guards
 * `>= MAX_SYSTEMS`). Taking it from the sentinel makes the two impossible to
 * disagree, which is better than asserting that they agree.
 */
inline constexpr uint8_t MAX_SYSTEMS = static_cast<uint8_t>(LedSystemType::Count);

class LEDStrip {
public:
    LEDStrip(uint8_t numLeds);
    void begin(TIM_HandleTypeDef *timer, const uint32_t channel);
    void setBrightness(uint8_t b);
    void clear() { _strip.clear(); }
    void applyCommand(const Command& cmd);   // queue-safe "set and forget"
    void tick();                             // call every loop: non-blocking
    void tickOneSystem(uint8_t idx);

private:
    uint8_t  _numLeds;
    WS2812Driver _strip;

    TIM_HandleTypeDef *_stripTimer;
    uint32_t _stripChannel;

    bool _processCmd = false;  // true if command received

    Command   _cmds[MAX_SYSTEMS];
    ModeState _states[MAX_SYSTEMS];

    // helpers
    int pctToIdx(uint8_t pct) const;
    void setAll(int start, int end, uint8_t r, uint8_t g, uint8_t b);
    void handleMode(uint8_t idx);

    /* Pattern engines, one per LedModeType. All non-blocking: each is called
     * once per tick() and advances its own ModeState. */
    void modeOff(uint8_t idx, int s, int e);
    void modeOn(uint8_t idx, int s, int e, uint8_t r, uint8_t g, uint8_t b);
    void modeBlink(uint8_t idx, int s, int e, uint8_t r, uint8_t g, uint8_t b,
                   uint8_t eye = 4, uint16_t speed = 50, uint16_t pause = 100);
    void modeFault(uint8_t idx, int s, int e, uint8_t r, uint8_t g, uint8_t b,
                   uint16_t speed = 250);
    void modeEmergencyMotors(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
    void modeEmergencyShutdown(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
    void modeAllOff(uint8_t idx);

};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////--CHECKS--DOWN--///////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

