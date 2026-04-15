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

#if INTEGRALL_MODULE_CELLULAR_ENABLED

namespace Integrall {

class CellularModule {
public:
    CellularModule() : _sim800(nullptr), _initialized(false) {}

    /**
     * Start the SIM800L Cellular Module
     * @param rxPin  ESP32 RX pin (connects to SIM TX)
     * @param txPin  ESP32 TX pin (connects to SIM RX)
     */
    bool begin(uint8_t rxPin = 16, uint8_t txPin = 17) {
        if (_initialized) return true;

#if defined(ESP32)
        // Use HardwareSerial 2 for ESP32
        _sim800 = new HardwareSerial(2);
        _sim800->begin(9600, SERIAL_8N1, rxPin, txPin);
#else
        return false; // ESP8266 support not fully implemented here yet
#endif

        if (!_sim800) return false;

        INTEGRALL_LOG_INFO("Cellular: Waiting 5s for network stabilization...");
        // This is a blocking delay on boot exactly as the user specified
        delay(5000); 

        INTEGRALL_LOG_INFO("Cellular: Testing GSM module...");
        sendAT("AT");
        sendAT("AT+CSQ");
        sendAT("AT+CREG?");
        sendAT("AT+CMGF=1"); // Text Mode
        
        delay(2000);
        
        _initialized = true;
        INTEGRALL_LOG_INFO("Cellular: Module Ready");
        return true;
    }

    /**
     * Send an AT command and wait for response 
     * Exact original logic from TestCode1.ino
     */
    void sendAT(String cmd) {
        if (!_initialized || !_sim800) return;
        
        _sim800->println(cmd);
        delay(1000);
        
        #if INTEGRALL_DEBUG_LEVEL > 2
        Serial.print(">> GSM: ");
        while (_sim800->available()) {
            Serial.write(_sim800->read());
        }
        Serial.println();
        #endif
    }

    /**
     * Send SMS Message
     * Exact original logic from TestCode1.ino
     * @param number  Target phone number
     * @param message Text payload
     */
    void sendSMS(const char* number, const char* message) {
        if (!_initialized || !_sim800) return;
        
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

        // Big delay waiting for network to process send
        delay(5000); 
        
        INTEGRALL_LOG_INFO("Cellular: SMS Sent");
    }

private:
    HardwareSerial* _sim800;
    bool _initialized;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_CELLULAR_ENABLED
#endif // INTEGRALL_CELLULAR_MODULE_H
