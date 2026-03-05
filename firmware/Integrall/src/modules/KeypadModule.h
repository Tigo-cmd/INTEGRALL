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
        , _last_key(NO_KEY) {
        memset(_buffer, 0, sizeof(_buffer));
        memset(_pin_buffer, 0, sizeof(_pin_buffer));
    }

    /**
     * Initialize with a custom pin layout
     * @param rowPins Array of row GPIO pins
     * @param colPins Array of column GPIO pins
     * @param rows Number of rows (default 4)
     * @param cols Number of columns (default 4)
     */
    bool begin(byte* rowPins, byte* colPins, byte rows = 4, byte cols = 4) {
        if (rows == 4 && cols == 4) {
            _keypad = new Keypad(makeKeymap(_DEFAULT_4x4_KEYMAP), rowPins, colPins, rows, cols);
        } else if (rows == 4 && cols == 3) {
            _keypad = new Keypad(makeKeymap(_DEFAULT_4x3_KEYMAP), rowPins, colPins, rows, cols);
        } else {
            return false;
        }
        _keypad->setDebounceTime(50);  // 50ms debounce built-in
        return true;
    }

    /**
     * Get the currently pressed key
     * Returns '\0' (NO_KEY) if no key is pressed
     */
    char getKey() {
        if (!_keypad) return '\0';
        char key = _keypad->getKey();
        if (key != NO_KEY) {
            _last_key = key;
        }
        return key;
    }

    /**
     * Get the last key that was pressed
     */
    char getLastKey() const { return _last_key; }

    /**
     * Buffer mode: Append pressed keys automatally
     * Returns the current buffered string
     */
    const char* captureString(uint8_t maxLen = 8) {
        char key = getKey();
        if (key != '\0') {
            if (key == '#') {  // '#' acts as Enter/Submit
                // Buffer is ready - do nothing, let user read _buffer
            } else if (key == '*') {  // '*' acts as Backspace 
                if (_bufferIndex > 0) {
                    _buffer[--_bufferIndex] = '\0';
                }
            } else if (_bufferIndex < min(maxLen, (uint8_t)(sizeof(_buffer) - 1))) {
                _buffer[_bufferIndex++] = key;
                _buffer[_bufferIndex] = '\0';
            }
        }
        return _buffer;
    }

    /**
     * Check if user pressed '#' to submit the buffer
     */
    bool isSubmitted() {
        char key = getKey();
        return (key == '#') && (_bufferIndex > 0);
    }

    /**
     * Clear the captured buffer
     */
    void clearBuffer() {
        memset(_buffer, 0, sizeof(_buffer));
        _bufferIndex = 0;
    }

    /**
     * Compare the buffered string to a password/PIN
     */
    bool checkPin(const char* correctPin) {
        return (strcmp(_buffer, correctPin) == 0);
    }

private:
    Keypad* _keypad;
    char _buffer[17];     // Capture buffer (max 16 chars + null)
    char _pin_buffer[17]; // Separate secure buffer for PIN comparison
    uint8_t _bufferIndex;
    char _last_key;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_KEYPAD_ENABLED
#endif // INTEGRALL_KEYPAD_MODULE_H
