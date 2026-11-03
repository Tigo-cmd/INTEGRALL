/**
 * KeypadModule.h
 * 
 * Matrix Keypad wrapper for Integrall Framework.
 * Hides row/col pin mapping, debouncing, and key scanning complexity.
 * 
 * Standard keymaps are built-in:
 *   - 4x4 matrix (0-9, A-D, *, #)
 *   - 4x3 matrix (0-9, *, #)
 */

#ifndef INTEGRALL_KEYPAD_MODULE_H
#define INTEGRALL_KEYPAD_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_KEYPAD_ENABLED

#include <Arduino.h>
#include <Keypad.h>  // Requires "Keypad" library by Mark Stanley

namespace Integrall {

// Default 4x4 keymap  
static char _DEFAULT_4x4_KEYMAP[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Default 4x3 keymap  
static char _DEFAULT_4x3_KEYMAP[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

class KeypadModule {
public:
    KeypadModule() 
        : _keypad(nullptr)
        , _bufferIndex(0)
        , _last_key(NO_KEY)
        , _just_pressed(NO_KEY)
        , _submitted(false) {
        memset(_buffer, 0, sizeof(_buffer));
        memset(_row_pins, 0, sizeof(_row_pins));
        memset(_col_pins, 0, sizeof(_col_pins));
    }

    ~KeypadModule() {
        if (_keypad) {
            delete _keypad;
            _keypad = nullptr;
        }
    }

    /**
     * Initialize with a custom pin layout
     * @param rowPins Array of row GPIO pins
     * @param colPins Array of column GPIO pins
     * @param rows Number of rows (default 4)
     * @param cols Number of columns (default 4)
     */
    bool begin(byte* rowPins, byte* colPins, byte rows = 4, byte cols = 4) {
        if (!rowPins || !colPins || rows != 4 || (cols != 4 && cols != 3)) {
            return false;
        }
        if (_keypad) {
            delete _keypad;
            _keypad = nullptr;
        }
        memcpy(_row_pins, rowPins, rows);
        memcpy(_col_pins, colPins, cols);

        if (cols == 4) {
            _keypad = new Keypad(makeKeymap(_DEFAULT_4x4_KEYMAP), _row_pins, _col_pins, rows, cols);
        } else {
            _keypad = new Keypad(makeKeymap(_DEFAULT_4x3_KEYMAP), _row_pins, _col_pins, rows, cols);
        }
        _keypad->setDebounceTime(50);  // 50ms debounce built-in
        clearBuffer();
        return true;
    }

    /**
     * Get the currently pressed key.
     * If captureString() was called earlier in the same loop iteration,
     * returns the key captured during that call once.
     * Returns '\0' (NO_KEY) if no key is pressed.
     */
    char getKey() {
        if (_just_pressed != NO_KEY) {
            char k = _just_pressed;
            _just_pressed = NO_KEY;
            return k;
        }
        return _readHardwareKey();
    }

    /**
     * Get the last key that was pressed
     */
    char getLastKey() const { return _last_key; }

    /**
     * Buffer mode: Append pressed keys automatically.
     * '#' marks the buffer as submitted, '*' acts as Backspace.
     * Returns the current buffered string.
     */
    const char* captureString(uint8_t maxLen = 8) {
        char key = _readHardwareKey();
        _just_pressed = key;
        if (key != NO_KEY) {
            if (key == '#') {  // '#' acts as Enter/Submit
                if (_bufferIndex > 0) {
                    _submitted = true;
                }
            } else if (key == '*') {  // '*' acts as Backspace 
                _submitted = false;
                if (_bufferIndex > 0) {
                    _buffer[--_bufferIndex] = '\0';
                }
            } else {
                _submitted = false;
                uint8_t limit = (maxLen < (sizeof(_buffer) - 1)) ? maxLen : (sizeof(_buffer) - 1);
                if (_bufferIndex < limit) {
                    _buffer[_bufferIndex++] = key;
                    _buffer[_bufferIndex] = '\0';
                }
            }
        }
        return _buffer;
    }

    /**
     * Check if user pressed '#' to submit a non-empty buffer
     */
    bool isSubmitted() {
        if (_submitted) {
            _submitted = false;
            if (_just_pressed == '#') {
                _just_pressed = NO_KEY;
            }
            return true;
        }
        char key = _readHardwareKey();
        if (key != NO_KEY) {
            _just_pressed = key;
            if (key == '#' && _bufferIndex > 0) {
                _just_pressed = NO_KEY;
                return true;
            }
        }
        return false;
    }

    /**
     * Clear the captured buffer
     */
    void clearBuffer() {
        memset(_buffer, 0, sizeof(_buffer));
        _bufferIndex = 0;
        _submitted = false;
        _just_pressed = NO_KEY;
    }

    /**
     * Compare the buffered string to a password/PIN
     */
    bool checkPin(const char* correctPin) const {
        if (!correctPin) return false;
        return (strcmp(_buffer, correctPin) == 0);
    }

private:
    char _readHardwareKey() {
        if (!_keypad) return NO_KEY;
        char key = _keypad->getKey();
        if (key != NO_KEY) {
            _last_key = key;
        }
        return key;
    }

    Keypad* _keypad;
    byte _row_pins[4];
    byte _col_pins[4];
    char _buffer[17];     // Capture buffer (max 16 chars + null)
    uint8_t _bufferIndex;
    char _last_key;
    char _just_pressed;
    bool _submitted;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_KEYPAD_ENABLED
#endif // INTEGRALL_KEYPAD_MODULE_H
