/**
 * BuzzerModule.h
 *
 * Professional non-blocking buzzer control for Integrall Framework.
 * Replaces delay() with a state machine for patterns and tones.
 */

#ifndef INTEGRALL_BUZZER_MODULE_H
#define INTEGRALL_BUZZER_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_BUZZER_ENABLED

#include <Arduino.h>

namespace Integrall {

class BuzzerModule {
public:
    BuzzerModule() : _pin(255), _initialized(false), _pattern_active(false), 
                     _pattern_count(0), _on_ms(0), _off_ms(0), _last_ms(0), 
                     _in_on_phase(false), _freq(2000), _is_success(false) {}

    bool begin(uint8_t pin) {
        _pin = pin;
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        _initialized = true;
        return true;
    }

    /**
     * Start a non-blocking beep pattern
     */
    void pattern(uint8_t times, uint32_t onMs = 150, uint32_t offMs = 100, uint16_t freq = 2000) {
        if (!_initialized) return;
        _is_success = false;
        _pattern_count = times;
        _on_ms = onMs;
        _off_ms = offMs;
        _freq = freq;
        _pattern_active = true;
        _last_ms = millis();
        _in_on_phase = true;
        tone(_pin, _freq);
    }

    /**
     * Play a specific frequency for a duration
     */
    void customTone(uint16_t freq, uint32_t durationMs) {
        pattern(1, durationMs, 0, freq);
    }

    void alert()   { if(!_pattern_active) pattern(3, 100, 80, 2500); }
    
    /**
     * Re-implemented descending/ascending success tone without blocking!
     */
    void success() { 
        if(!_pattern_active) {
            _is_success = true;
            pattern(2, 100, 50, 1500); // Start with 1500Hz
        }
    }

    void failure() { if(!_pattern_active) pattern(1, 500, 0, 800); }
    void beep(uint32_t ms = 150) { if(!_pattern_active) pattern(1, ms, 0, 2000); }

    void off() {
        _pattern_active = false;
        if (_initialized) noTone(_pin);
    }

    /**
     * Call in loop() to process patterns
     */
    void handle() {
        if (!_pattern_active || !_initialized) return;

        unsigned long now = millis();
        if (_in_on_phase) {
            if (now - _last_ms >= _on_ms) {
                noTone(_pin);
                _in_on_phase = false;
                _last_ms = now;
                if (_pattern_count > 0) _pattern_count--;
                if (_pattern_count == 0) {
                    _pattern_active = false;
                    _is_success = false;
                }
            }
        } else if (_pattern_count > 0) {
            if (now - _last_ms >= _off_ms) {
                // Feature: Handle frequency jump for melodic tones (Success)
                if (_is_success && _pattern_count == 1) {
                    _freq = 2500; // Jump to high tone for the second beep
                }

                tone(_pin, _freq);
                _in_on_phase = true;
                _last_ms = now;
            }
        }
    }

    bool isActive() const { return _pattern_active; }

private:
    uint8_t  _pin;
    bool     _initialized;
    bool     _pattern_active;
    uint8_t  _pattern_count;
    uint16_t _freq;
    uint32_t _on_ms;
    uint32_t _off_ms;
    unsigned long _last_ms;
    bool     _in_on_phase;
    bool     _is_success; // Flag to indicate a multi-tone sequence
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_BUZZER_ENABLED
#endif // INTEGRALL_BUZZER_MODULE_H
