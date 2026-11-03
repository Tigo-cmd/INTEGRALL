/**
 * CellularModule.h
 * 
 * Logic for Cellular / GSM connectivity (e.g., SIM800L).
 * Handles sending emergency SMS via AT commands using precise timing.
 */

#ifndef INTEGRALL_CELLULAR_MODULE_H
#define INTEGRALL_CELLULAR_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"

#if INTEGRALL_MODULE_CELLULAR_ENABLED

namespace Integrall {

class CellularModule {
public:
    CellularModule() : _sim800(nullptr), _ownsSerial(false), _initialized(false) {}

    ~CellularModule() {
        if (_ownsSerial && _sim800) {
            delete _sim800;
            _sim800 = nullptr;
        }
    }

    /**
     * Start the SIM800L Cellular Module
     * @param rxPin  MCU RX pin (connects to SIM TX)
     * @param txPin  MCU TX pin (connects to SIM RX)
     */
    bool begin(uint8_t rxPin = 16, uint8_t txPin = 17) {
        if (_initialized) return true;

#if defined(ESP32)
        if (_ownsSerial && _sim800) {
            delete _sim800;
        }
        _sim800 = new HardwareSerial(2);
        _ownsSerial = true;
        _sim800->begin(9600, SERIAL_8N1, rxPin, txPin);
#else
        (void)rxPin;
        (void)txPin;
        return false; // HardwareSerial pin remapping requires ESP32
#endif

        if (!_sim800) return false;

        _initialized = true;

        INTEGRALL_LOG_INFO("Cellular: Waiting 5s for network stabilization...");
        delay(5000); 

        INTEGRALL_LOG_INFO("Cellular: Testing GSM module...");
        sendAT("AT");
        sendAT("AT+CSQ");
        sendAT("AT+CREG?");
        sendAT("AT+CMGF=1"); // Text Mode
        
        delay(2000);
        
        INTEGRALL_LOG_INFO("Cellular: Module Ready");
        return true;
    }

    /**
     * Send an AT command and wait for response 
     */
    void sendAT(const char* cmd) {
        if (!_initialized || !_sim800 || !cmd) return;
        
        _sim800->println(cmd);
        delay(1000);
        
        #if INTEGRALL_DEBUG_LEVEL > 2
        if (Logger::ready()) {
            Serial.print(F(">> GSM: "));
            while (_sim800->available()) {
                Serial.write(_sim800->read());
            }
            Serial.println();
        } else {
            while (_sim800->available()) {
                _sim800->read();
            }
        }
        #else
        while (_sim800->available()) {
            _sim800->read();
        }
        #endif
    }

    /**
     * Send SMS Message
     * @param number  Target phone number
     * @param message Text payload
     */
    void sendSMS(const char* number, const char* message) {
        if (!_initialized || !_sim800 || !number || !message) return;
        
        INTEGRALL_LOG_INFO_VAL("Cellular: Sending SMS to ", number);

        _sim800->println("AT+CMGF=1");
        delay(1000);

        _sim800->print("AT+CMGS=\"");
        _sim800->print(number);
        _sim800->println("\"");

        delay(1000);

        _sim800->print(message);

        delay(500);

        _sim800->write(26); // Ctrl+Z to send

        // Wait for network to process send
        delay(5000); 
        
        INTEGRALL_LOG_INFO("Cellular: SMS Sent");
    }

private:
    HardwareSerial* _sim800;
    bool _ownsSerial;
    bool _initialized;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_CELLULAR_ENABLED
#endif // INTEGRALL_CELLULAR_MODULE_H
