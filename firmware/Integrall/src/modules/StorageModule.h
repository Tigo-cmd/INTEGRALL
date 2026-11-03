/**
 * StorageModule.h
 * 
 * Handles data persistence: SD cards (SPI), EEPROM (Flash), and basic file system operations.
 * Optimized for Espressif and AVR platforms.
 */

#ifndef INTEGRALL_STORAGE_MODULE_H
#define INTEGRALL_STORAGE_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_STORAGE_ENABLED

#include <EEPROM.h>

// Optional: include SD support if user has the library installed
#if __has_include(<SD.h>)
  #include <SD.h>
  #include <SPI.h>
  #define INTEGRALL_SD_AVAILABLE 1
#else
  #define INTEGRALL_SD_AVAILABLE 0
#endif

namespace Integrall {

class StorageModule {
public:
    StorageModule() : _sdInitialized(false) {}

    /**
     * Start the EEPROM with a fixed size (usually 512 bytes on ESP32/ESP8266; no-op on AVR)
     */
    void beginEEPROM(uint16_t size = 512) {
        #if defined(ESP32) || defined(ESP8266)
        EEPROM.begin(size);
        #else
        (void)size;
        #endif
    }

    /**
     * Write simple integer to EEPROM
     */
    void writeInt(int addr, int data) {
        EEPROM.put(addr, data);
        #if defined(ESP32) || defined(ESP8266)
        EEPROM.commit();
        #endif
    }

    /**
     * Read simple integer from EEPROM
     */
    int readInt(int addr) {
        int val = 0;
        EEPROM.get(addr, val);
        return val;
    }

    /**
     * Write string to SD card file (append mode)
     */
#if INTEGRALL_SD_AVAILABLE
    bool logToFile(const char* path, const char* message, uint8_t csPin = 5) {
        if (!path || !message) return false;
        if (!_sdInitialized) {
            if (SD.begin(csPin)) _sdInitialized = true;
            else return false;
        }

        #ifdef FILE_APPEND
        File file = SD.open(path, FILE_APPEND);
        #else
        File file = SD.open(path, FILE_WRITE);
        #endif
        if (file) {
            file.println(message);
            file.close();
            return true;
        }
        return false;
    }
    
    /**
     * Delete a file from SD card
     */
    bool deleteFile(const char* path) {
        if (!_sdInitialized || !path) return false;
        return SD.remove(path);
    }
#endif

private:
    bool _sdInitialized;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_STORAGE_ENABLED
#endif // INTEGRALL_STORAGE_MODULE_H
