/**
 * RGBModule.h
 *
 * Simple RGB LED control for Integrall Framework.
 * Supports common anode/cathode LEDs (3 separate pins).
 */

#ifndef INTEGRALL_RGB_MODULE_H
#define INTEGRALL_RGB_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_RGB_ENABLED

#include <Arduino.h>

namespace Integrall {

class RGBModule {
public:
    RGBModule() : _rPin(255), _gPin(255), _bPin(255),
                  _commonAnode(false), _initialized(false),
                  _lastBlink(0), _blinkState(false) {}

    /**
     * Initialize RGB LED
     * @param rPin         Red pin
     * @param gPin         Green pin
     * @param bPin         Blue pin
     * @param commonAnode  true = common anode (logic inverted), false = common cathode
     */
    bool begin(uint8_t rPin, uint8_t gPin, uint8_t bPin, bool commonAnode = false) {
        _rPin = rPin; _gPin = gPin; _bPin = bPin;
        _commonAnode = commonAnode;
        pinMode(_rPin, OUTPUT);
        pinMode(_gPin, OUTPUT);
        pinMode(_bPin, OUTPUT);
        _initialized = true;
        off();
        return true;
    }

    /**
     * Set color by RGB values (0-255 each)
     */
    void set(uint8_t r, uint8_t g, uint8_t b) {
        _blinking = false;
        _write(r, g, b);
    }

    /**
     * Set color by name
     */
    void setColor(const char* colorName) {
        if (!colorName) { off(); return; }
        if      (strcmp(colorName, "red")    == 0)    set(255, 0,   0);
        else if (strcmp(colorName, "green")  == 0)    set(0,   255, 0);
        else if (strcmp(colorName, "blue")   == 0)    set(0,   0,   255);
        else if (strcmp(colorName, "yellow") == 0)    set(255, 255, 0);
        else if (strcmp(colorName, "cyan")   == 0)    set(0,   255, 255);
        else if (strcmp(colorName, "white")  == 0)    set(255, 255, 255);
        else if (strcmp(colorName, "orange") == 0)    set(255, 100, 0);
        else if (strcmp(colorName, "purple") == 0)    set(150, 0,   255);
        else                                          off();
    }

    /**
     * Turn off the LED
     */
    void off() {
        _blinking = false;
        _write(0, 0, 0);
    }

    /**
     * Start (or keep) a non-blocking blink. Safe to call every loop: calling
     * again with the same colour/interval does not restart the cycle.
     * Any set()/setColor()/off() call stops blinking.
     */
    void blink(uint8_t r, uint8_t g, uint8_t b, uint32_t intervalMs = 500) {
        if (_blinking && _br == r && _bg == g && _bb == b && _bi == intervalMs) return;
        _br = r; _bg = g; _bb = b; _bi = intervalMs ? intervalMs : 500;
        _blinking = true;
        _blinkState = true;
        _lastBlink = millis();
        _write(r, g, b);
    }

    /** Legacy name for blink(). */
    void updateBlink(uint8_t r, uint8_t g, uint8_t b, uint32_t intervalMs = 500) { blink(r, g, b, intervalMs); }

    /** Called by System::handle(). */
    void handle() {
        if (!_blinking || !_initialized) return;
        const unsigned long now = millis();
        if (now - _lastBlink < _bi) return;
        _lastBlink = now;
        _blinkState = !_blinkState;
        if (_blinkState) _write(_br, _bg, _bb); else _write(0, 0, 0);
    }

private:
    void _write(uint8_t r, uint8_t g, uint8_t b) {
        if (!_initialized) return;
        analogWrite(_rPin, _commonAnode ? 255 - r : r);
        analogWrite(_gPin, _commonAnode ? 255 - g : g);
        analogWrite(_bPin, _commonAnode ? 255 - b : b);
    }

    bool     _blinking = false;
    uint8_t  _br = 0, _bg = 0, _bb = 0;
    uint32_t _bi = 500;

    uint8_t _rPin, _gPin, _bPin;
    bool    _commonAnode;
    bool    _initialized;
    unsigned long _lastBlink;
    bool    _blinkState;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_RGB_ENABLED
#endif // INTEGRALL_RGB_MODULE_H
