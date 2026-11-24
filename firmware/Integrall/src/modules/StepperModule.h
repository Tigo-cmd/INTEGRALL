/**
 * StepperModule.h
 *
 * Non-blocking stepper motor driver for the Integrall Framework.
 * step()/moveDegrees() only set a target; handle() emits one step at a time
 * when it is due, so the rest of the system keeps running while the motor
 * turns. No external library needed.
 *
 * Supported hardware:
 *  - 4-wire unipolar/bipolar via ULN2003 / L298N (e.g. 28BYJ-48):
 *      begin(2048, IN1, IN3, IN2, IN4)   // same pin order as Arduino's Stepper lib
 *  - STEP/DIR driver boards (A4988, DRV8825, TMC2208):
 *      beginDriver(stepPin, dirPin, enablePin, 200)
 *
 * Top speed is limited by how often loop() runs; keep loop() free of long
 * work for smooth motion.
 */

#ifndef INTEGRALL_STEPPER_MODULE_H
#define INTEGRALL_STEPPER_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_STEPPER_ENABLED

#include <Arduino.h>

namespace Integrall {

class StepperModule {
public:
    StepperModule() {}

    /** 4-wire motor (Arduino Stepper-compatible coil sequence). */
    void begin(int stepsPerRev, int p1, int p2, int p3, int p4) {
        _mode = MODE_4WIRE;
        _pins[0] = p1; _pins[1] = p2; _pins[2] = p3; _pins[3] = p4;
        for (uint8_t i = 0; i < 4; i++) pinMode(_pins[i], OUTPUT);
        _setStepsPerRev(stepsPerRev);
        _coils(_phase);
    }

    /** STEP/DIR driver board. enablePin = -1 if not wired (active-LOW enable). */
    void beginDriver(int stepPin, int dirPin, int enablePin = -1, int stepsPerRev = 200) {
        _mode = MODE_DRIVER;
        _pins[0] = stepPin; _pins[1] = dirPin; _pins[2] = enablePin;
        pinMode(stepPin, OUTPUT); digitalWrite(stepPin, LOW);
        pinMode(dirPin, OUTPUT);
        if (enablePin >= 0) { pinMode(enablePin, OUTPUT); digitalWrite(enablePin, LOW); }
        _setStepsPerRev(stepsPerRev);
    }

    /** Speed in RPM (applies to the current and future moves). */
    void setSpeed(int rpm) {
        if (rpm <= 0) return;
        _rpm = rpm;
        _recalc();
    }

    /** Queue a relative move of N steps (negative = reverse). Returns immediately. */
    void step(long steps) {
        if (_mode == MODE_NONE) return;
        if (!isMoving()) _last_step_us = micros() - _interval_us;   // first step right away
        _target += steps;
        _enable(true);
    }

    /** Queue a relative move in degrees. */
    void moveDegrees(long deg) { step((deg * (long)_steps_per_rev) / 360L); }

    /** Queue an absolute move to a step position. */
    void moveTo(long position) { step(position - _position); }

    /** Stop as soon as possible (keeps holding torque). */
    void stop() { _target = _position; }

    /** De-energise the coils / driver (motor can be turned by hand, saves power). */
    void release() {
        if (_mode == MODE_4WIRE) for (uint8_t i = 0; i < 4; i++) digitalWrite(_pins[i], LOW);
        _enable(false);
    }

    bool isMoving() const      { return _position != _target; }
    long position() const      { return _position; }
    long distanceToGo() const  { return _target - _position; }
    void setPosition(long p)   { _position = _target = p; }

    /** Call every loop. Emits at most one step per call. */
    void handle() {
        if (_mode == MODE_NONE || _position == _target) return;
        const unsigned long now = micros();
        if (now - _last_step_us < _interval_us) return;
        _last_step_us = now;
        const int dir = (_target > _position) ? 1 : -1;
        if (_mode == MODE_4WIRE) {
            _phase = (uint8_t)((_phase + (dir > 0 ? 1 : 3)) & 3);
            _coils(_phase);
        } else {
            digitalWrite(_pins[1], dir > 0 ? HIGH : LOW);
            digitalWrite(_pins[0], HIGH);
            delayMicroseconds(2);              // A4988 min pulse 1 us, DRV8825 1.9 us
            digitalWrite(_pins[0], LOW);
        }
        _position += dir;
    }

private:
    enum Mode : uint8_t { MODE_NONE, MODE_4WIRE, MODE_DRIVER };
    Mode          _mode = MODE_NONE;
    int           _pins[4] = {-1, -1, -1, -1};
    int           _steps_per_rev = 2048;
    int           _rpm = 10;
    unsigned long _interval_us = 2929;
    unsigned long _last_step_us = 0;
    long          _position = 0;
    long          _target = 0;
    uint8_t       _phase = 0;

    void _setStepsPerRev(int spr) { _steps_per_rev = (spr > 0) ? spr : 2048; _recalc(); }
    void _recalc() { _interval_us = 60UL * 1000UL * 1000UL / (unsigned long)_steps_per_rev / (unsigned long)_rpm; }

    void _coils(uint8_t phase) {
        static const uint8_t seq[4] = {0b1010, 0b0110, 0b0101, 0b1001};
        for (uint8_t i = 0; i < 4; i++) digitalWrite(_pins[i], (seq[phase] >> (3 - i)) & 1);
    }

    void _enable(bool on) {
        if (_mode == MODE_DRIVER && _pins[2] >= 0) digitalWrite(_pins[2], on ? LOW : HIGH);
    }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_STEPPER_ENABLED
#endif // INTEGRALL_STEPPER_MODULE_H
