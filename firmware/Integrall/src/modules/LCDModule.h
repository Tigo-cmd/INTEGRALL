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
    LCDModule() : _lcd(nullptr), _initialized(false) {
        for (int i = 0; i < 2; i++) {
            _row_text[i] = "";
            _scroll_pos[i] = 0;
            _last_scroll[i] = 0;
        }
    }

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
        
        _cols = cols;
        _rows = rows;
        _initialized = true;
        return true;
    }

    /**
     * Smart Print: Automatically detects if text is too long for the screen.
     * If short, it prints static text. If long, it starts scrolling in the loop.
     */
    void print(const char* text, uint8_t col = 0, uint8_t row = 0) {
        if (!_initialized || row >= 2) return;
        
        String input = String(text);
        
        // If it's a new string or short, reset the static/scroll state
        if (input != _row_text[row] || input.length() <= _cols) {
            _row_text[row] = input;
            _scroll_pos[row] = 0;
            _last_scroll[row] = 0;
            
            _lcd->setCursor(col, row);
            if (input.length() <= _cols) {
                // Short text: clear the row and print static
                char padding[_cols + 1];
                memset(padding, ' ', _cols);
                padding[_cols] = '\0';
                _lcd->print(padding); // Pre-clear
                _lcd->setCursor(col, row);
                _lcd->print(text);
            }
        }
    }

    /**
     * Mandatory Handle: Processes automatic scrolling for long rows.
     * Call this in the main loop.
     */
    void handle(uint16_t speed_ms = 350) {
        if (!_initialized) return;

        for (uint8_t r = 0; r < _rows; r++) {
            if (_row_text[r].length() > _cols) {
                if (millis() - _last_scroll[r] > speed_ms) {
                    _last_scroll[r] = millis();
                    
                    String window = "";
                    int len = _row_text[r].length();
                    for (int i = 0; i < _cols; i++) {
                        int idx = (_scroll_pos[r] + i) % (len + 2);
                        if (idx < len) window += _row_text[r][idx];
                        else window += " ";
                    }
                    
                    _lcd->setCursor(0, r);
                    _lcd->print(window.c_str());
                    
                    _scroll_pos[r]++;
                    if (_scroll_pos[r] >= (len + 2)) _scroll_pos[r] = 0;
                }
            }
        }
    }

    /**
     * Professional Scrolling: Non-blocking text scroll on a specific row
     * Call this in the main loop to keep the text moving.
     */
    void scrollText(const char* text, uint8_t row = 0, uint16_t speed_ms = 300) {
        if (!_initialized) return;
        
        static unsigned long last_scroll = 0;
        static int scroll_pos = 0;
        
        String display_text = String(text);
        if (display_text.length() <= _cols) {
            print(text, 0, row);
            return;
        }

        if (millis() - last_scroll > speed_ms) {
            last_scroll = millis();
            
            // Create a window of text
            String window = "";
            for (int i = 0; i < _cols; i++) {
                int idx = (scroll_pos + i) % (display_text.length() + 2);
                if (idx < display_text.length()) {
                    window += display_text[idx];
                } else {
                    window += " "; // Padding between loops
                }
            }
            
            print(window.c_str(), 0, row);
            scroll_pos++;
            if (scroll_pos >= (display_text.length() + 2)) scroll_pos = 0;
        }
    }

    /**
     * Liquid Design: Control the visual "life" of the screen
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
        _lcd->createChar(slot, charMap);
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
    uint8_t _cols, _rows;
    String _row_text[2];
    int _scroll_pos[2];
    unsigned long _last_scroll[2];
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_LCD_ENABLED
#endif // INTEGRALL_LCD_MODULE_H
