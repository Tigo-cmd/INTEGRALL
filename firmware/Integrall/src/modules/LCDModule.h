/**
 * LCDModule.h
 *
 * I2C character LCD (HD44780 + PCF8574 backpack) for the Integrall Framework.
 *
 *  - Screen buffer flushed in handle(): a row is only re-sent over I2C when
 *    its contents actually changed, so calling print() every loop is cheap
 *    and flicker-free.
 *  - print() at column 0 replaces the whole line (classic "line" behaviour);
 *    print() at any other column writes in place, so
 *        print("Temp:", 0, 0); print("25.1", 10, 0);   // both stay visible
 *  - Text longer than the row scrolls automatically (non-blocking).
 *  - Presence detection: begin() returns false if nothing answers on I2C;
 *    address 0 auto-detects 0x27 (PCF8574) / 0x3F (PCF8574A).
 *  - No heap allocation.
 */

#ifndef INTEGRALL_LCD_MODULE_H
#define INTEGRALL_LCD_MODULE_H

#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"
#include "../core/Util.h"

#if INTEGRALL_MODULE_LCD_ENABLED

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <new>

#ifndef INTEGRALL_LCD_MAX_COLS
  #define INTEGRALL_LCD_MAX_COLS 20
#endif
#ifndef INTEGRALL_LCD_SCROLL_MAX
  #if defined(__AVR__)
    #define INTEGRALL_LCD_SCROLL_MAX 20   // keep Uno/Nano RAM free
  #else
    #define INTEGRALL_LCD_SCROLL_MAX 64   // longest scrolling text per row
  #endif
#endif

namespace Integrall {

class LCDModule {
public:
    LCDModule() { _resetBuffers(); }
    ~LCDModule() { _destroy(); }

    /**
     * Start the LCD.
     * @param address I2C address, or 0 to auto-detect 0x27 / 0x3F
     * @return true if the LCD answered on the I2C bus
     */
    bool begin(uint8_t address = 0, uint8_t cols = 16, uint8_t rows = 2) {
        if (rows > 4) rows = 4;
        if (cols > INTEGRALL_LCD_MAX_COLS) cols = INTEGRALL_LCD_MAX_COLS;
        if (address == 0) {
            if (Util::i2cPresent(0x27)) address = 0x27;
            else if (Util::i2cPresent(0x3F)) address = 0x3F;
        }
        if (address == 0 || !Util::i2cPresent(address)) {
            INTEGRALL_LOG_WARN("LCD not found on I2C (tried 0x27/0x3F) - check SDA/SCL wiring");
            _destroy();
            return false;
        }
        if (_lcd && _address == address && _cols == cols && _rows == rows) return true;

        _destroy();
        _lcd = new (_storage) LiquidCrystal_I2C(address, cols, rows);
        _lcd->init();
        _lcd->backlight();
        _lcd->clear();
        _address = address; _cols = cols; _rows = rows;
        _resetBuffers();
        INTEGRALL_LOG_INFO_VAL("LCD ready at I2C 0x", String(address, HEX));
        return true;
    }

    bool isPresent() const  { return _lcd != nullptr; }
    uint8_t address() const { return _address; }

    /**
     * Write text at (col,row); shown on the next handle().
     *  - col == 0: replaces the whole line.
     *  - col  > 0: writes in place; the rest of the row is untouched.
     * Text that doesn't fit from `col` to the row end turns the row into a ticker.
     */
    void print(const char* text, uint8_t col = 0, uint8_t row = 0) {
        if (!_lcd || !text || row >= _rows || col >= _cols) return;
        const size_t len = strlen(text);
        if (col + len > _cols) {
            _setScroll(row, text, _scroll_speed[row]);
            return;
        }
        if (_scroll_len[row]) { _scroll_len[row] = 0; _setRow(row, 0, "", 0, true); }
        _setRow(row, col, text, len, col == 0);
    }

    /** Scrolling ticker on a row (non-blocking; animated by handle()). */
    void scrollText(const char* text, uint8_t row = 0, uint16_t speed_ms = 300) {
        if (!_lcd || !text || row >= _rows) return;
        const size_t len = strlen(text);
        if (len <= _cols) {                    // fits: just show it
            _scroll_len[row] = 0;
            _setRow(row, 0, text, len, true);
            return;
        }
        _setScroll(row, text, speed_ms);
    }

    /** Call every loop: animates tickers and sends changed rows. Never blocks. */
    void handle() {
        if (!_lcd) return;
        const unsigned long now = millis();
        for (uint8_t r = 0; r < _rows; r++) {
            if (_scroll_len[r] && now - _scroll_last[r] >= _scroll_speed[r]) {
                _scroll_last[r] = now;
                _scroll_pos[r] = (uint8_t)((_scroll_pos[r] + 1) % (_scroll_len[r] + 3));
                _renderScroll(r);
            }
            _flushRow(r);
        }
    }

    /** Send pending changes now instead of on the next handle(). */
    void flush() { if (_lcd) for (uint8_t r = 0; r < _rows; r++) _flushRow(r); }

    void setCursorVisible(bool visible, bool blink = false) {
        if (!_lcd) return;
        if (visible) _lcd->cursor(); else _lcd->noCursor();
        if (blink) _lcd->blink(); else _lcd->noBlink();
    }

    /** Define a custom glyph (slot 0-7); print it with a char of that code. */
    void createCustomChar(uint8_t slot, uint8_t charMap[]) {
        if (_lcd) _lcd->createChar(slot & 0x07, charMap);
    }

    void clear() {
        if (!_lcd) return;
        _lcd->clear();
        _resetBuffers();
    }

    void setBacklight(bool on) {
        if (!_lcd) return;
        if (on) _lcd->backlight(); else _lcd->noBacklight();
    }

    /** Raw driver for advanced use (nullptr if the LCD is absent). */
    LiquidCrystal_I2C* getRaw() { return _lcd; }

private:
    LiquidCrystal_I2C* _lcd = nullptr;
    alignas(LiquidCrystal_I2C) uint8_t _storage[sizeof(LiquidCrystal_I2C)];
    uint8_t _address = 0, _cols = 16, _rows = 2;

    char     _want[4][INTEGRALL_LCD_MAX_COLS];  // desired screen contents
    uint8_t  _dirty = 0;                        // bit r set = row r may have changed
    uint32_t _sent[4];                          // fingerprint of what is on the glass

    char          _scroll_text[4][INTEGRALL_LCD_SCROLL_MAX + 1];
    uint8_t       _scroll_len[4];
    uint8_t       _scroll_pos[4];
    uint16_t      _scroll_speed[4];
    unsigned long _scroll_last[4];

    void _destroy() { if (_lcd) { _lcd->~LiquidCrystal_I2C(); _lcd = nullptr; } }

    void _resetBuffers() {
        _dirty = 0;
        for (uint8_t r = 0; r < 4; r++) {
            memset(_want[r], ' ', INTEGRALL_LCD_MAX_COLS);
            _sent[r] = _rowHash(r);                 // glass is blank after clear()/init()
            _scroll_text[r][0] = '\0';
            _scroll_len[r] = 0; _scroll_pos[r] = 0;
            _scroll_speed[r] = 300; _scroll_last[r] = 0;
        }
    }

    /**
     * Write `len` chars at `col` into the buffer; with blankRest, every cell
     * after the text becomes a space. Marks the row dirty only on a real change.
     */
    void _setRow(uint8_t r, uint8_t col, const char* text, size_t len, bool blankRest) {
        bool changed = false;
        for (uint8_t c = col; c < _cols; c++) {
            char ch;
            if (c < col + len) ch = text[c - col];
            else if (blankRest) ch = ' ';
            else break;
            if (_want[r][c] != ch) { _want[r][c] = ch; changed = true; }
        }
        if (changed) _dirty |= (uint8_t)(1u << r);
    }

    /** FNV-1a over the visible row: lets us skip rows whose net content didn't change. */
    uint32_t _rowHash(uint8_t r) const {
        uint32_t h = 2166136261UL;
        for (uint8_t c = 0; c < _cols; c++) { h ^= (uint8_t)_want[r][c]; h *= 16777619UL; }
        return h;
    }

    void _flushRow(uint8_t r) {
        if (!(_dirty & (1u << r))) return;
        _dirty &= (uint8_t)~(1u << r);
        const uint32_t h = _rowHash(r);
        if (h == _sent[r]) return;                  // e.g. label+value re-printed each loop
        _lcd->setCursor(0, r);
        for (uint8_t c = 0; c < _cols; c++) _lcd->write((uint8_t)_want[r][c]);
        _sent[r] = h;
    }

    void _setScroll(uint8_t r, const char* text, uint16_t speed) {
        size_t len = strlen(text);
        if (len > INTEGRALL_LCD_SCROLL_MAX) len = INTEGRALL_LCD_SCROLL_MAX;
        _scroll_speed[r] = speed ? speed : 300;
        if (_scroll_len[r] == len && strncmp(_scroll_text[r], text, len) == 0) return;  // keep animating
        memcpy(_scroll_text[r], text, len);
        _scroll_text[r][len] = '\0';
        _scroll_len[r] = (uint8_t)len;
        _scroll_pos[r] = 0;
        _scroll_last[r] = millis();
        _renderScroll(r);
    }

    void _renderScroll(uint8_t r) {
        const uint8_t span = _scroll_len[r] + 3;   // 3 spaces between repeats
        for (uint8_t c = 0; c < _cols; c++) {
            uint8_t i = (uint8_t)((_scroll_pos[r] + c) % span);
            _want[r][c] = (i < _scroll_len[r]) ? _scroll_text[r][i] : ' ';
        }
        _dirty |= (uint8_t)(1u << r);
    }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_LCD_ENABLED
#endif // INTEGRALL_LCD_MODULE_H
