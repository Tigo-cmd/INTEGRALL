/**
 * StorageModule.h
 * 
 * Handles data persistence: SD cards (SPI), EEPROM (Flash), and basic file system operations.
 * Optimized for Espressif SPIFFS/LittleFS where available.
 */

#ifndef INTEGRALL_STORAGE_MODULE_H
#define INTEGRALL_STORAGE_MODULE_H

#include <Arduino.h>
#include <EEPROM.h>
#include "../config/IntegrallConfig.h"

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
     * Start the EEPROM with a fixed size (usually 512 bytes on Arduino/ESP)
     */
    void beginEEPROM(uint16_t size = 512) {
        EEPROM.begin(size);
    }

    /**
     * Write simple integer to EEPROM
     */
    void writeInt(int addr, int data) {
        EEPROM.put(addr, data);
        EEPROM.commit();
    }

    /**
     * Read simple integer from EEPROM
     */
    int readInt(int addr) {
        int val;
        EEPROM.get(addr, val);
        return val;
    }

    /**
     * Write string to SD card file (append mode)
     */
#if INTEGRALL_SD_AVAILABLE
    bool logToFile(const char* path, const char* message, uint8_t csPin = 5) {
        if (!_sdInitialized) {
            if (SD.begin(csPin)) _sdInitialized = true;
            else return false;
        }

        File file = SD.open(path, FILE_APPEND);
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
        if (!_sdInitialized) return false;
        return SD.remove(path);
    }
#endif

private:
    bool _sdInitialized;
};

} // namespace Integrall

#endif // INTEGRALL_STORAGE_MODULE_H
