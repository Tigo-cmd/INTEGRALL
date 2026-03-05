/**
 * BlinkerModule.h
 * 
 * Simple non-blocking LED/pin blinker for Integrall Framework.
 * Replaces the traditional "Blink" example which uses blocking delay().
 */

#ifndef INTEGRALL_BLINKER_MODULE_H
#define INTEGRALL_BLINKER_MODULE_H

#include <Arduino.h>

namespace Integrall {

class BlinkerModule {
public:
    BlinkerModule() : _pin(255), _interval(1000), _last_ms(0), _state(false), _active(false) {}

    /**
     * Start blinking a pin
     * @param pin      GPIO pin (e.g., LED_BUILTIN)
     * @param interval Time in ms between flips
     */
    void begin(uint8_t pin, uint32_t interval = 1000) {
        _pin = pin;
        _interval = interval;
        _active = true;
        _last_ms = millis();
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
    }

    /**
     * Stop the blinker
     */
    void stop() {
        _active = false;
        if (_pin != 255) digitalWrite(_pin, LOW);
    }

    /**
     * Update the blinker state (call in loop)
     */
    void handle() {
        if (!_active || _pin == 255) return;

        if (millis() - _last_ms >= _interval) {
            _state = !_state;
            digitalWrite(_pin, _state ? HIGH : LOW);
            _last_ms = millis();
        }
    }

    bool isActive() const { return _active; }

private:
    uint8_t _pin;
    uint32_t _interval;
    unsigned long _last_ms;
    bool _state;
    bool _active;
};

} // namespace Integrall

#endif // INTEGRALL_BLINKER_MODULE_H
