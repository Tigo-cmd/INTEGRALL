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
                  _commonAnode(false), _initialized(false) {}

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
        off();
        _initialized = true;
        return true;
    }

    /**
     * Set color by RGB values (0-255 each)
     */
    void set(uint8_t r, uint8_t g, uint8_t b) {
        if (!_initialized) return;
        analogWrite(_rPin, _commonAnode ? 255 - r : r);
        analogWrite(_gPin, _commonAnode ? 255 - g : g);
        analogWrite(_bPin, _commonAnode ? 255 - b : b);
    }

    /**
     * Set color by name
     */
    void setColor(const char* colorName) {
        if      (strcmp(colorName, "red")    == 0)    set(255, 0,   0);
        else if (strcmp(colorName, "green")  == 0)    set(0,   255, 0);
        else if (strcmp(colorName, "blue")   == 0)    set(0,   0,   255);
        else if (strcmp(colorName, "yellow") == 0)    set(255, 255, 0);
        else if (strcmp(colorName, "cyan")   == 0)    set(0,   255, 255);
        else if (strcmp(colorName, "white")  == 0)    set(255, 255, 255);
        else if (strcmp(colorName, "orange") == 0)    set(255, 100, 0);
        else if (strcmp(colorName, "purple") == 0)    set(150, 0,   255);
        else                                           off();
    }

    /**
     * Turn off the LED
     */
    void off() {
        if (!_initialized) return;
        uint8_t v = _commonAnode ? 255 : 0;
        analogWrite(_rPin, v);
        analogWrite(_gPin, v);
        analogWrite(_bPin, v);
    }

    /**
     * Non-blocking blink (call in loop)
     */
    void updateBlink(uint8_t r, uint8_t g, uint8_t b, uint32_t intervalMs = 500) {
        static unsigned long last = 0;
        static bool state = false;
        if (millis() - last > intervalMs) {
            state = !state;
            if (state) set(r, g, b); else off();
            last = millis();
        }
    }

private:
    uint8_t _rPin, _gPin, _bPin;
    bool    _commonAnode;
    bool    _initialized;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_RGB_ENABLED
#endif // INTEGRALL_RGB_MODULE_H
