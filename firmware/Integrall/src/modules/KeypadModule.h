/**
 * KeypadModule.h
 *
 * 4x4 / 4x3 matrix keypad for the Integrall Framework.
 *
 * The matrix is scanned exactly once per loop in handle(); every consumer
 * (getKey, captureString, isSubmitted) reads the result of that scan, so a
 * key can never be "stolen" by one call and lost to another.
 *
 * Input buffer: digits/letters append, '*' = backspace, '#' = submit.
 * Needs the "Keypad" library (Mark Stanley / Alexander Brevig).
 */

#ifndef INTEGRALL_KEYPAD_MODULE_H
#define INTEGRALL_KEYPAD_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_KEYPAD_ENABLED

#include <Arduino.h>
#include <Keypad.h>
#include <new>

namespace Integrall {

class KeypadModule {
public:
    KeypadModule() { clearBuffer(); }
    ~KeypadModule() { _destroy(); }

    /**
     * @param rowPins 4 row GPIOs
     * @param colPins 4 (or 3) column GPIOs
     */
    bool begin(byte* rowPins, byte* colPins, byte rows = 4, byte cols = 4) {
        if (!rowPins || !colPins || rows != 4 || (cols != 4 && cols != 3)) return false;
        _destroy();
        memcpy(_row_pins, rowPins, rows);
        memcpy(_col_pins, colPins, cols);
        static const char k4[4][4] = {{'1','2','3','A'},{'4','5','6','B'},{'7','8','9','C'},{'*','0','#','D'}};
        for (uint8_t r = 0; r < 4; r++)
            for (uint8_t c = 0; c < cols; c++)
                _keymap[r * cols + c] = k4[r][c];   // 4x3 pads simply drop the A-D column
        _keypad = new (_storage) Keypad(_keymap, _row_pins, _col_pins, rows, cols);
        _keypad->setDebounceTime(30);
        clearBuffer();
        return true;
    }

    /** Scan the matrix once. Called by System::handle(); never blocks. */
    void handle() {
        if (!_keypad) return;
        char k = _keypad->getKey();
        if (k == NO_KEY) return;
        _last_key = k;
        _pending = k;             // for getKey()

        if (k == '#') {
            if (_len > 0) _submitted = true;
        } else if (k == '*') {
            _submitted = false;
            if (_len > 0) _buffer[--_len] = '\0';
        } else if (_len < _max_len) {
            _submitted = false;
            _buffer[_len++] = k;
            _buffer[_len] = '\0';
        }
    }

    /** Key pressed since the last call ('\0' if none). Each press is returned once. */
    char getKey() { char k = _pending; _pending = NO_KEY; return k; }

    char getLastKey() const { return _last_key; }

    /** Current input buffer (updated by handle()). @param maxLen buffer limit (1-16). */
    const char* captureString(uint8_t maxLen = 8) {
        _max_len = (maxLen == 0) ? 1 : (maxLen > MAX_BUF ? MAX_BUF : maxLen);
        if (_len > _max_len) { _len = _max_len; _buffer[_len] = '\0'; }
        return _buffer;
    }

    /** true once after '#' is pressed with a non-empty buffer. */
    bool isSubmitted() {
        if (!_submitted) return false;
        _submitted = false;
        return true;
    }

    void clearBuffer() {
        memset(_buffer, 0, sizeof(_buffer));
        _len = 0;
        _submitted = false;
    }

    /** Constant-time compare of the buffer with a PIN. */
    bool checkPin(const char* correctPin) const {
        if (!correctPin) return false;
        const size_t n = strlen(correctPin);
        uint8_t diff = (uint8_t)(n != _len);
        for (size_t i = 0; i < MAX_BUF; i++) {
            const char a = (i < _len) ? _buffer[i] : 0;
            const char b = (i < n) ? correctPin[i] : 0;
            diff |= (uint8_t)(a ^ b);
        }
        return diff == 0;
    }

    bool isReady() const { return _keypad != nullptr; }

private:
    static const uint8_t MAX_BUF = 16;

    Keypad* _keypad = nullptr;
    alignas(Keypad) uint8_t _storage[sizeof(Keypad)];
    char    _keymap[16];
    byte    _row_pins[4] = {0};
    byte    _col_pins[4] = {0};
    char    _buffer[MAX_BUF + 1];
    uint8_t _len = 0;
    uint8_t _max_len = 8;
    char    _last_key = NO_KEY;
    char    _pending = NO_KEY;
    bool    _submitted = false;

    void _destroy() { if (_keypad) { _keypad->~Keypad(); _keypad = nullptr; } }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_KEYPAD_ENABLED
#endif // INTEGRALL_KEYPAD_MODULE_H
