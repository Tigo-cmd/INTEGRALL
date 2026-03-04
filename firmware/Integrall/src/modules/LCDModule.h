/**
 * LCDModule.h
 * 
 * Simple I2C LCD wrapper for Integrall Framework.
 * Hides Wire initialization and boilerplate setup.
 */

#ifndef INTEGRALL_LCD_MODULE_H
#define INTEGRALL_LCD_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_LCD_ENABLED

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

namespace Integrall {

class LCDModule {
public:
    LCDModule() : _lcd(nullptr), _initialized(false) {}

    /**
     * Start the LCD with default address 0x27 and size 16x2
     */
    bool begin(uint8_t address = 0x27, uint8_t cols = 16, uint8_t rows = 2) {
        if (_initialized) return true;

        // Auto-start I2C if not already started
        Wire.begin(); 
        
        _lcd = new LiquidCrystal_I2C(address, cols, rows);
        _lcd->init();
        _lcd->backlight();
        _lcd->clear();
        
        _initialized = true;
        return true;
    }

    void print(const char* text, uint8_t col = 0, uint8_t row = 0) {
        if (!_initialized) return;
        _lcd->setCursor(col, row);
        _lcd->print(text);
    }

    void clear() {
        if (!_initialized) return;
        _lcd->clear();
    }

    void setBacklight(bool on) {
        if (!_initialized) return;
        if (on) _lcd->backlight();
        else _lcd->noBacklight();
    }

    // Access to raw object if advanced users need it
    LiquidCrystal_I2C* getRaw() { return _lcd; }

private:
    LiquidCrystal_I2C* _lcd;
    bool _initialized;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_LCD_ENABLED
#endif // INTEGRALL_LCD_MODULE_H
