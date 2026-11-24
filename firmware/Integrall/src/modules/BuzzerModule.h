/**
 * BuzzerModule.h
 *
 * Non-blocking buzzer control for the Integrall Framework: patterns and tones
 * run from handle(), never delay().
 *
 * Two kinds of buzzer:
 *  - passive (default): a bare piezo/magnetic buzzer, driven with tone() at a frequency
 *  - active:            a module with its own oscillator (usually marked "+" and
 *                       sounds on plain 3.3/5 V); driven HIGH/LOW, frequency ignored
 *        enableBuzzer(pin, true)
 *
 * on()/off() latch the buzzer; beep()/pattern()/alert()... play and stop by themselves
 * (and cancel a latched on()).
 */

#ifndef INTEGRALL_BUZZER_MODULE_H
#define INTEGRALL_BUZZER_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_BUZZER_ENABLED

#include <Arduino.h>

namespace Integrall {

class BuzzerModule {
public:
    BuzzerModule() {}

    /** @param active true for an active buzzer module (on/off, no tone). */
    bool begin(uint8_t pin, bool active = false) {
        _pin = pin;
        _active = active;
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        _initialized = true;
        return true;
    }

    /** Sound continuously until off() (or a beep/pattern). */
    void on(uint16_t freq = 2000) {
        if (!_initialized) return;
        _pattern_active = false;
        _is_success = false;
        _continuous = true;
        _freq = freq;
        _start();
    }

    /** Start a non-blocking beep pattern. */
    void pattern(uint8_t times, uint32_t onMs = 150, uint32_t offMs = 100, uint16_t freq = 2000) {
        if (!_initialized || times == 0) return;
        _continuous = false;
        _is_success = false;
        _pattern_count = times;
        _on_ms = onMs;
        _off_ms = offMs;
        _freq = freq;
        _pattern_active = true;
        _last_ms = millis();
        _in_on_phase = true;
        _start();
    }

    /** Play a frequency for a duration (passive buzzers; active ones just sound). */
    void customTone(uint16_t freq, uint32_t durationMs) { pattern(1, durationMs, 0, freq); }

    void alert()   { if (!_pattern_active) pattern(3, 100, 80, 2500); }

    /** Two rising tones (passive buzzer). */
    void success() {
        if (!_pattern_active) {
            pattern(2, 100, 50, 1500);
            _is_success = true;
        }
    }

    void failure() { if (!_pattern_active) pattern(1, 500, 0, 800); }
    void beep(uint32_t ms = 150) { if (!_pattern_active) pattern(1, ms, 0, 2000); }

    void off() {
        _pattern_active = false;
        _continuous = false;
        if (_initialized) _stop();
    }

    /** Call in loop() to run patterns. */
    void handle() {
        if (!_pattern_active || !_initialized) return;
        const unsigned long now = millis();
        if (_in_on_phase) {
            if (now - _last_ms >= _on_ms) {
                _stop();
                _in_on_phase = false;
                _last_ms = now;
                if (_pattern_count > 0) _pattern_count--;
                if (_pattern_count == 0) {
                    _pattern_active = false;
                    _is_success = false;
                }
            }
        } else if (_pattern_count > 0 && now - _last_ms >= _off_ms) {
            if (_is_success && _pattern_count == 1) _freq = 2500;   // second, higher note
            _start();
            _in_on_phase = true;
            _last_ms = now;
        }
    }

    /** true while making (or about to make) sound: a pattern is running or on() is latched. */
    bool isActive() const { return _pattern_active || _continuous; }
    /** true while latched on with on(). */
    bool isOn() const { return _continuous; }

private:
    uint8_t  _pin = 255;
    bool     _initialized = false;
    bool     _active = false;          // active buzzer module: HIGH = sound
    bool     _pattern_active = false;
    bool     _continuous = false;
    uint8_t  _pattern_count = 0;
    uint16_t _freq = 2000;
    uint32_t _on_ms = 0;
    uint32_t _off_ms = 0;
    unsigned long _last_ms = 0;
    bool     _in_on_phase = false;
    bool     _is_success = false;

    void _start() { if (_active) digitalWrite(_pin, HIGH); else tone(_pin, _freq); }
    void _stop()  { if (_active) digitalWrite(_pin, LOW);  else noTone(_pin); }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_BUZZER_ENABLED
#endif // INTEGRALL_BUZZER_MODULE_H
