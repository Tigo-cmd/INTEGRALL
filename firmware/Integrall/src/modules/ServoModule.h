/**
 * ServoModule.h
 * 
 * Simple wrapper for Servo motors in the Integrall Framework.
 * On ESP32, this handles PWM timers behind the scenes.
 * On standard Arduinos, it uses the standard Servo library.
 */

#ifndef INTEGRALL_SERVO_MODULE_H
#define INTEGRALL_SERVO_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_SERVO_ENABLED

#include <Arduino.h>

#if defined(ESP32)
  #include <ESP32Servo.h> // Recommended for ESP32
#else
  #include <Servo.h>
#endif

namespace Integrall {

#ifndef INTEGRALL_MAX_SERVOS
#define INTEGRALL_MAX_SERVOS 4
#endif

class ServoModule {
public:
    ServoModule() {
        for (int i = 0; i < INTEGRALL_MAX_SERVOS; i++) {
            _attached[i] = false;
            _current_angle[i] = 0;
            _target_angle[i] = 0;
            _speed_ms[i] = 15;
            _last_move[i] = 0;
            _mode[i] = 0;
            _sweep_dir[i] = 1;
        }
    }

    /**
     * Attach a servo to a specific GPIO pin, with an optional starting angle
     * Returns the servo index (0 to INTEGRALL_MAX_SERVOS-1), or -1 if full.
     */
    int attach(uint8_t pin, int startAngle = -1) {
        for (int i = 0; i < INTEGRALL_MAX_SERVOS; i++) {
            if (!_attached[i]) {
                _servo[i].attach(pin);
                _attached[i] = true;
                if (startAngle >= 0) {
                    set(i, startAngle);
                }
                return i;
            }
        }
        return -1; // Max servos reached
    }

    /**
     * Hard set the servo angle (0 to 180) instantly
     */
    void set(int index, uint8_t angle) {
        if (index < 0 || index >= INTEGRALL_MAX_SERVOS || !_attached[index]) return;
        if (angle > 180) angle = 180;
        _servo[index].write(angle);
        _current_angle[index] = angle;
        _mode[index] = 0; // Stop animations
    }

    /**
     * Map an analog pin directly to servo angle (0-180)
     */
    void setFromAnalog(int index, uint8_t pin) {
        if (index < 0 || index >= INTEGRALL_MAX_SERVOS || !_attached[index]) return;
        int val = analogRead(pin);
        int angle = map(val, 0, 1023, 0, 180);
        set(index, angle);
    }

    /**
     * Start a non-blocking back-and-forth sweep
     */
    void sweep(int index, uint32_t speed_ms = 15) {
        if (index < 0 || index >= INTEGRALL_MAX_SERVOS || !_attached[index]) return;
        _speed_ms[index] = speed_ms;
        _mode[index] = 1;
    }

    /**
     * Start a non-blocking ease (slow move) to a target angle
     */
    void easeTo(int index, uint8_t angle, uint32_t speed_ms = 15) {
        if (index < 0 || index >= INTEGRALL_MAX_SERVOS || !_attached[index]) return;
        if (angle > 180) angle = 180;
        _target_angle[index] = angle;
        _speed_ms[index] = speed_ms;
        _mode[index] = 2;
    }

    /**
     * Background handler for animations
     */
    void handle() {
        for (int i = 0; i < INTEGRALL_MAX_SERVOS; i++) {
            if (!_attached[i] || _mode[i] == 0) continue;

            if (millis() - _last_move[i] > _speed_ms[i]) {
                if (_mode[i] == 1) { // Sweep
                    _current_angle[i] += _sweep_dir[i];
                    if (_current_angle[i] >= 180 || _current_angle[i] <= 0) _sweep_dir[i] *= -1;
                    _servo[i].write(_current_angle[i]);
                } 
                else if (_mode[i] == 2) { // EaseTo
                    if (_current_angle[i] < _target_angle[i]) _current_angle[i]++;
                    else if (_current_angle[i] > _target_angle[i]) _current_angle[i]--;
                    else _mode[i] = 0; // Reached target, stop easing
                    
                    _servo[i].write(_current_angle[i]);
                }
                _last_move[i] = millis();
            }
        }
    }

private:
#if defined(ESP32)
    Servo _servo[INTEGRALL_MAX_SERVOS];
#else
    Servo _servo[INTEGRALL_MAX_SERVOS];
#endif
    bool _attached[INTEGRALL_MAX_SERVOS];
    int _current_angle[INTEGRALL_MAX_SERVOS];
    int _target_angle[INTEGRALL_MAX_SERVOS];
    uint32_t _speed_ms[INTEGRALL_MAX_SERVOS];
    unsigned long _last_move[INTEGRALL_MAX_SERVOS];
    int _mode[INTEGRALL_MAX_SERVOS]; // 0: static, 1: sweep, 2: easeTo
    int _sweep_dir[INTEGRALL_MAX_SERVOS];
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_SERVO_ENABLED
#endif // INTEGRALL_SERVO_MODULE_H

