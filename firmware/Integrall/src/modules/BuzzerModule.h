/**
 * BuzzerModule.h
 *
 * Simple buzzer control for Integrall Framework.
 * Supports single beeps, multi-pattern beeps, and custom tones.
 * Works on both ESP32 (LEDC) and standard Arduino (tone()).
 */

#ifndef INTEGRALL_BUZZER_MODULE_H
#define INTEGRALL_BUZZER_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_BUZZER_ENABLED

#include <Arduino.h>

namespace Integrall {

class BuzzerModule {
public:
    BuzzerModule() : _pin(255), _initialized(false) {}

    bool begin(uint8_t pin) {
        _pin = pin;
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        _initialized = true;
        return true;
    }

    /**
     * Single beep for a duration
     */
    void beep(uint32_t durationMs = 200) {
        if (!_initialized) return;
#if defined(ESP32)
        tone(_pin, 2000, durationMs);
#else
        tone(_pin, 2000, durationMs);
#endif
        delay(durationMs + 10);
    }

    /**
     * Beep N times with configurable on/off timing
     */
    void pattern(uint8_t times, uint32_t onMs = 150, uint32_t offMs = 100) {
        if (!_initialized) return;
        for (uint8_t i = 0; i < times; i++) {
            tone(_pin, 2000, onMs);
            delay(onMs + offMs);
        }
    }

    /**
     * Beep at a specific frequency
     */
    void customTone(uint16_t freq, uint32_t durationMs) {
        if (!_initialized) return;
        tone(_pin, freq, durationMs);
        delay(durationMs + 10);
    }

    /**
     * Alert pattern: 3 rapid beeps (alarm/danger)
     */
    void alert() { pattern(3, 100, 80); }

    /**
     * Success pattern: 2 ascending beeps
     */
    void success() {
        if (!_initialized) return;
        tone(_pin, 1500, 100); delay(150);
        tone(_pin, 2500, 150); delay(200);
    }

    /**
     * Failure pattern: 1 long low beep
     */
    void failure() {
        if (!_initialized) return;
        tone(_pin, 800, 500);
        delay(550);
    }

    void off() {
        if (!_initialized) return;
        noTone(_pin);
        digitalWrite(_pin, LOW);
    }

private:
    uint8_t _pin;
    bool    _initialized;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_BUZZER_ENABLED
#endif // INTEGRALL_BUZZER_MODULE_H
