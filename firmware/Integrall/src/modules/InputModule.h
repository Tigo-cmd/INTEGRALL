/**
 * InputModule.h
 * 
 * Handles user interaction devices: Buttons, Rotary Encoders, Joysticks, RFID, and IR.
 * Features per-pin software debouncing and state management.
 */

#ifndef INTEGRALL_INPUT_MODULE_H
#define INTEGRALL_INPUT_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_INPUT_ENABLED

#if __has_include(<SPI.h>) && __has_include(<MFRC522.h>)
  #include <SPI.h>
  #include <MFRC522.h>
  #define INTEGRALL_RFID_AVAILABLE 1
#else
  #define INTEGRALL_RFID_AVAILABLE 0
#endif

#if __has_include(<IRremote.h>)
  #include <IRremote.h>
  #define INTEGRALL_IR_AVAILABLE 1
#else
  #define INTEGRALL_IR_AVAILABLE 0
#endif

namespace Integrall {

#ifndef INTEGRALL_MAX_INPUT_PINS
#define INTEGRALL_MAX_INPUT_PINS 8
#endif

class InputModule {
public:
    InputModule() : _debounceDelay(50), _lastEncoderCLK(-1) {
        for (uint8_t i = 0; i < INTEGRALL_MAX_INPUT_PINS; i++) {
            _btnPins[i] = 255;
            _btnState[i] = false;
            _lastBtnReading[i] = false;
            _lastDebounceTime[i] = 0;
        }
    }

    /**
     * Read button with per-pin software debouncing (returns true on press edge)
     */
    bool buttonPressed(uint8_t pin, bool activeLow = true) {
        int slot = _getButtonSlot(pin, activeLow);
        if (slot < 0) return false;

        bool raw = (digitalRead(pin) == HIGH);
        bool pressed = activeLow ? !raw : raw;
        unsigned long now = millis();

        if (pressed != _lastBtnReading[slot]) {
            _lastDebounceTime[slot] = now;
            _lastBtnReading[slot] = pressed;
        }

        if ((now - _lastDebounceTime[slot]) >= _debounceDelay) {
            if (pressed != _btnState[slot]) {
                _btnState[slot] = pressed;
                if (_btnState[slot]) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Read Joystick Axis (Normalized -100 to 100)
     */
    int readJoystickAxis(uint8_t pin, int centerValue = 512, int deadzone = 50) {
        int val = analogRead(pin);
        #if defined(ESP32)
        // Normalize 12-bit ADC (0-4095) to 10-bit (0-1023) when using 10-bit center values
        if (centerValue <= 1023) {
            val = map(val, 0, 4095, 0, 1023);
        }
        int maxVal = (centerValue <= 1023) ? 1023 : 4095;
        #else
        int maxVal = 1023;
        #endif

        int diff = val - centerValue;
        if (abs(diff) < deadzone) return 0;

        int mapped = 0;
        if (diff > 0) {
            mapped = map(val, centerValue + deadzone, maxVal, 0, 100);
        } else {
            mapped = map(val, 0, centerValue - deadzone, -100, 0);
        }
        return constrain(mapped, -100, 100);
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
        pinMode(pinCLK, INPUT);
        pinMode(pinDT, INPUT);
        int clk = digitalRead(pinCLK);
        int dir = 0;
        if (clk != _lastEncoderCLK && _lastEncoderCLK != -1) {
            if (digitalRead(pinDT) != clk) dir = 1;
            else dir = -1;
        }
        _lastEncoderCLK = clk;
        return dir;
    }

    /**
     * Read RFID UID (MFRC522)
     */
#if INTEGRALL_RFID_AVAILABLE
    String readRFID(uint8_t ss = 5, uint8_t rst = 0) {
        if (!_rfid || _rfidSS != ss || _rfidRST != rst) {
            if (_rfid) delete _rfid;
            SPI.begin();
            _rfid = new MFRC522(ss, rst);
            _rfid->PCD_Init();
            _rfidSS = ss;
            _rfidRST = rst;
        }
        if (!_rfid->PICC_IsNewCardPresent() || !_rfid->PICC_ReadCardSerial()) return "";
        String uid = "";
        for (byte i = 0; i < _rfid->uid.size; i++) {
            if (_rfid->uid.uidByte[i] < 0x10) uid += "0";
            uid += String(_rfid->uid.uidByte[i], HEX);
        }
        _rfid->PICC_HaltA();
        return uid;
    }
#endif

    /**
     * Read IR Remote Code
     */
#if INTEGRALL_IR_AVAILABLE
    uint32_t readIR(uint8_t pin) {
        #if defined(IRREMOTE_VERSION_MAJOR) && (IRREMOTE_VERSION_MAJOR >= 3)
        if (!_irInited || _irPin != pin) {
            IrReceiver.begin(pin, DISABLE_LED_FEEDBACK);
            _irInited = true;
            _irPin = pin;
        }
        if (IrReceiver.decode()) {
            uint32_t val = IrReceiver.decodedIRData.decodedRawData;
            IrReceiver.resume();
            return val;
        }
        return 0;
        #else
        static IRrecv irrecv(pin);
        static decode_results results;
        if (!_irInited) {
            irrecv.enableIRIn();
            _irInited = true;
        }
        if (irrecv.decode(&results)) {
            uint32_t val = results.value;
            irrecv.resume();
            return val;
        }
        return 0;
        #endif
    }
#endif

private:
    int _getButtonSlot(uint8_t pin, bool activeLow) {
        for (uint8_t i = 0; i < INTEGRALL_MAX_INPUT_PINS; i++) {
            if (_btnPins[i] == pin) return i;
        }
        for (uint8_t i = 0; i < INTEGRALL_MAX_INPUT_PINS; i++) {
            if (_btnPins[i] == 255) {
                _btnPins[i] = pin;
                pinMode(pin, activeLow ? INPUT_PULLUP : INPUT);
                bool raw = (digitalRead(pin) == HIGH);
                bool pressed = activeLow ? !raw : raw;
                _btnState[i] = pressed;
                _lastBtnReading[i] = pressed;
                _lastDebounceTime[i] = millis();
                return i;
            }
        }
        return -1;
    }

    uint8_t _btnPins[INTEGRALL_MAX_INPUT_PINS];
    bool _btnState[INTEGRALL_MAX_INPUT_PINS];
    bool _lastBtnReading[INTEGRALL_MAX_INPUT_PINS];
    unsigned long _lastDebounceTime[INTEGRALL_MAX_INPUT_PINS];
    unsigned long _debounceDelay;
    int _lastEncoderCLK;

#if INTEGRALL_RFID_AVAILABLE
    MFRC522* _rfid = nullptr;
    uint8_t _rfidSS = 255;
    uint8_t _rfidRST = 255;
#endif

#if INTEGRALL_IR_AVAILABLE
    bool _irInited = false;
    uint8_t _irPin = 255;
#endif
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_INPUT_ENABLED
#endif // INTEGRALL_INPUT_MODULE_H
