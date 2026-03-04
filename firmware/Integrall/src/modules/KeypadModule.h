/**
 * KeypadModule.h
 * 
 * Matrix keypad support for Integrall Framework
 * v0.1: Basic interface (full implementation in v0.2)
 */

#ifndef INTEGRALL_KEYPAD_MODULE_H
#define INTEGRALL_KEYPAD_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_KEYPAD_ENABLED

#include <Arduino.h>
#include <Keypad.h>  // Requires Keypad library by Mark Stanley

namespace Integrall {

class KeypadModule {
public:
    KeypadModule();
    bool begin(char* userKeymap, byte* row, byte* col, byte numRows, byte numCols);
    char getKey();
    void setDebounceTime(uint16_t ms);
    void setHoldTime(uint16_t ms);
    bool isPressed(char keyChar);
    String getString();
    void clearBuffer();
    
private:
    Keypad* _keypad;
    char _buffer[16];
    uint8_t _bufferIndex;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_KEYPAD_ENABLED

#endif // INTEGRALL_KEYPAD_MODULE_H
