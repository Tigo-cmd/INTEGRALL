/**
 * Logger.h
 * 
 * Centralized logging utility for Integrall Framework
 * Provides consistent, memory-efficient logging across all modules
 */

#ifndef INTEGRALL_LOGGER_H
#define INTEGRALL_LOGGER_H

#include "../config/IntegrallConfig.h"

namespace Integrall {

class Logger {
public:
    enum Level {
        SILENT = 0,
        ERROR = 1,
        WARNING = 2,
        INFO = 3,
        VERBOSE = 4
    };

    // Initialize serial with specified baud rate
    static void begin(unsigned long baud = 115200) {
        #if INTEGRALL_DEBUG_LEVEL > 0
        if (!Serial) {
            Serial.begin(baud);
            // Wait a moment for serial to initialize (optional)
            delay(100);
        }
        #endif
    }

    // Check if logger is ready
    static bool ready() {
        #if INTEGRALL_DEBUG_LEVEL > 0
        return Serial;
        #else
        return false;
        #endif
    }

    // Template for printing values with F() macro optimization
    template<typename T>
    static void print(const __FlashStringHelper* prefix, T message) {
        #if INTEGRALL_DEBUG_LEVEL > 0
        if (Serial) {
            Serial.print(prefix);
            Serial.println(message);
        }
        #endif
    }

    // Print raw string from Flash
    static void println(const __FlashStringHelper* msg) {
        #if INTEGRALL_DEBUG_LEVEL > 0
        if (Serial) Serial.println(msg);
        #endif
    }

    // Print to Flash string without newline
    static void print(const __FlashStringHelper* msg) {
        #if INTEGRALL_DEBUG_LEVEL > 0
        if (Serial) Serial.print(msg);
        #endif
    }

    // Memory reporting (ESP32 specific)
    static void printMemoryStats() {
        #if INTEGRALL_DEBUG_LEVEL >= 3 && defined(ESP32)
        if (Serial) {
            Serial.print(F("[MEM] Free heap: "));
            Serial.print(ESP.getFreeHeap());
            Serial.print(F(" bytes | Min free: "));
            Serial.print(ESP.getMinFreeHeap());
            Serial.println(F(" bytes"));
        }
        #endif
    }
};

} // namespace Integrall

#endif // INTEGRALL_LOGGER_H
