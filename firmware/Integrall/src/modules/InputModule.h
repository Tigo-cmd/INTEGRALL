/**
 * InputModule.h
 * 
 * Handles user interaction devices: Buttons, Rotary Encoders, and Joysticks.
 * Features software debouncing and state management.
 */

#ifndef INTEGRALL_INPUT_MODULE_H
#define INTEGRALL_INPUT_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

namespace Integrall {

class InputModule {
public:
    InputModule() : _lastButtonState(HIGH), _lastDebounceTime(0), _debounceDelay(50) {}

    /**
     * Read button with basic software debouncing
     */
    bool buttonPressed(uint8_t pin, bool activeLow = true) {
        pinMode(pin, activeLow ? INPUT_PULLUP : INPUT);
        bool reading = digitalRead(pin);
        
        if (activeLow) reading = !reading;

        if (reading != _lastButtonState) {
            _lastDebounceTime = millis();
        }

        if ((millis() - _lastDebounceTime) > _debounceDelay) {
            if (reading != _buttonState) {
                _buttonState = reading;
                if (_buttonState == HIGH) return true;
            }
        }

        _lastButtonState = reading;
        return false;
    }

    /**
     * Read Joystick Axis (Normalized -100 to 100)
     */
    int readJoystickAxis(uint8_t pin, int centerValue = 512, int deadzone = 50) {
        int val = analogRead(pin);
        int diff = val - centerValue;
        
        if (abs(diff) < deadzone) return 0;
        
        if (diff > 0) {
            return map(val, centerValue + deadzone, 1023, 0, 100);
        } else {
            return map(val, 0, centerValue - deadzone, -100, 0);
        }
    }

    /**
     * Read Touch Sensor (TTP223)
     */
    bool touchActive(uint8_t pin) {
        pinMode(pin, INPUT);
        return digitalRead(pin) == HIGH;
    }

    /**
     * Read Rotary Encoder (Direction: 1, -1, or 0)
     */
    int readEncoder(uint8_t pinCLK, uint8_t pinDT) {
        static int lastCLK = -1;
        pinMode(pinCLK, INPUT);
        pinMode(pinDT, INPUT);
        int clk = digitalRead(pinCLK);
        int dir = 0;
        if (clk != lastCLK && lastCLK != -1) {
            if (digitalRead(pinDT) != clk) dir = 1;
            else dir = -1;
        }
        lastCLK = clk;
        return dir;
    }

    /**
     * Read RFID UID (Legacy MFRC522)
     * Requires MFRC522.h
     */
#if __has_include(<MFRC522.h>)
    String readRFID(uint8_t ss = 5, uint8_t rst = 0) {
        #include <MFRC522.h>
        MFRC522 rfid(ss, rst);
        static bool _rfidInited = false;
        if (!_rfidInited) { rfid.PCD_Init(); _rfidInited = true; }
        if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return "";
        String uid = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
            uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
            uid += String(rfid.uid.uidByte[i], HEX);
        }
        rfid.PICC_HaltA();
        return uid;
    }
#endif

    /**
     * Read IR Remote Code
     * Requires IRremote.h
     */
#if __has_include(<IRremote.h>)
    uint32_t readIR(uint8_t pin) {
        #include <IRremote.h>
        static IRrecv irrecv(pin);
        static decode_results results;
        static bool _irInited = false;
        if (!_irInited) { irrecv.enableIRIn(); _irInited = true; }
        if (irrecv.decode(&results)) {
            uint32_t val = results.value;
            irrecv.resume();
            return val;
        }
        return 0;
    }
#endif

private:
    bool _buttonState;
    bool _lastButtonState;
    unsigned long _lastDebounceTime;
    unsigned long _debounceDelay;
};

} // namespace Integrall

#endif // INTEGRALL_INPUT_MODULE_H
