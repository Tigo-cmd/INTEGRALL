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
    LCDModule()
        : _lcd(nullptr)
        , _initialized(false)
        , _address(0x27)
        , _cols(16)
        , _rows(2) {
        for (int i = 0; i < 4; i++) {
            _row_text[i] = "";
            _row_col[i] = 0;
            _scroll_pos[i] = 0;
            _last_scroll[i] = 0;
            _row_speed[i] = 350;
        }
    }

    ~LCDModule() {
        if (_lcd) {
            delete _lcd;
            _lcd = nullptr;
        }
    }

    /**
     * Start the LCD with default address 0x27 and size 16x2.
     * Can be called again via enableLCD() to reconfigure address or dimensions.
     */
    bool begin(uint8_t address = 0x27, uint8_t cols = 16, uint8_t rows = 2) {
        if (rows > 4) rows = 4;
        if (cols > 40) cols = 40;
        if (_initialized && _address == address && _cols == cols && _rows == rows) {
            return true;
        }

        if (_lcd) {
            delete _lcd;
            _lcd = nullptr;
        }

        Wire.begin(); 
        
        _lcd = new LiquidCrystal_I2C(address, cols, rows);
        _lcd->init();
        _lcd->backlight();
        _lcd->clear();
        
        _address = address;
        _cols = cols;
        _rows = rows;
        _initialized = true;
        for (int i = 0; i < 4; i++) {
            _row_text[i] = "";
            _row_col[i] = 0;
            _scroll_pos[i] = 0;
            _last_scroll[i] = 0;
        }
        return true;
    }

    /**
     * Smart Print: Automatically detects if text is too long for the screen.
     * Caches row text so repeated calls in loop() with identical content
     * do not spam the I2C bus or cause flicker.
     */
    void print(const char* text, uint8_t col = 0, uint8_t row = 0) {
        if (!_initialized || !text || row >= _rows) return;
        
        String input(text);
        if (input == _row_text[row] && col == _row_col[row]) {
            return; // Unchanged — skip redundant I2C write
        }

        _row_text[row] = input;
        _row_col[row] = col;
        _scroll_pos[row] = 0;
        _last_scroll[row] = millis();

        if (col + input.length() <= _cols) {
            char line[41];
            uint8_t width = (_cols <= 40) ? _cols : 40;
            memset(line, ' ', width);
            memcpy(line + col, text, input.length());
            line[width] = '\0';
            _lcd->setCursor(0, row);
            _lcd->print(line);
        } else {
            // Render initial window immediately
            _renderScrollWindow(row);
        }
    }

    /**
     * Mandatory Handle: Processes automatic scrolling for long rows.
     * Call this in the main loop.
     */
    void handle(uint16_t default_speed_ms = 350) {
        if (!_initialized) return;

        unsigned long now = millis();
        for (uint8_t r = 0; r < _rows; r++) {
            if (_row_text[r].length() > _cols) {
                uint16_t spd = (_row_speed[r] > 0) ? _row_speed[r] : default_speed_ms;
                if (now - _last_scroll[r] >= spd) {
                    _last_scroll[r] = now;
                    _renderScrollWindow(r);
                }
            }
        }
    }

    /**
     * Non-blocking text scroll on a specific row.
     * Safe to call in loop(); updates per-row scroll speed and text.
     */
    void scrollText(const char* text, uint8_t row = 0, uint16_t speed_ms = 300) {
        if (!_initialized || !text || row >= _rows) return;
        _row_speed[row] = speed_ms;
        print(text, 0, row);
        handle(speed_ms);
    }

    /**
     * Control the visual cursor state
     */
    void setCursorVisible(bool visible, bool blink = false) {
        if (!_initialized) return;
        if (visible) _lcd->cursor(); else _lcd->noCursor();
        if (blink) _lcd->blink(); else _lcd->noBlink();
    }

    /**
     * Create custom symbols (e.g. Heart, Battery, Signal)
     * @param slot 0-7 available slots
     * @param charMap 8-byte array defining pixels
     */
    void createCustomChar(uint8_t slot, uint8_t charMap[]) {
        if (!_initialized) return;
        _lcd->createChar(slot & 0x07, charMap);
    }

    void clear() {
        if (!_initialized) return;
        _lcd->clear();
        for (int i = 0; i < 4; i++) {
            _row_text[i] = "";
            _row_col[i] = 0;
            _scroll_pos[i] = 0;
        }
    }

    void setBacklight(bool on) {
        if (!_initialized) return;
        if (on) _lcd->backlight();
        else _lcd->noBacklight();
    }

    // Access to raw object if advanced users need it
    LiquidCrystal_I2C* getRaw() { return _lcd; }

private:
    void _renderScrollWindow(uint8_t r) {
        int len = _row_text[r].length();
        if (len <= 0) return;
        char window[41];
        uint8_t width = (_cols <= 40) ? _cols : 40;
        for (uint8_t i = 0; i < width; i++) {
            int idx = (_scroll_pos[r] + i) % (len + 2);
            window[i] = (idx < len) ? _row_text[r][idx] : ' ';
        }
        window[width] = '\0';
        _lcd->setCursor(0, r);
        _lcd->print(window);
        _scroll_pos[r]++;
        if (_scroll_pos[r] >= (len + 2)) _scroll_pos[r] = 0;
    }

    LiquidCrystal_I2C* _lcd;
    bool _initialized;
    uint8_t _address;
    uint8_t _cols, _rows;
    String _row_text[4];
    uint8_t _row_col[4];
    int _scroll_pos[4];
    unsigned long _last_scroll[4];
    uint16_t _row_speed[4];
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_LCD_ENABLED
#endif // INTEGRALL_LCD_MODULE_H
