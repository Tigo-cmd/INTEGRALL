/**
 * InputModule.h
 *
 * Buttons, joysticks, touch pads, rotary encoders, RFID and IR remotes.
 *
 *  - Buttons: per-pin debounce, press-edge detection (call every loop).
 *  - Rotary encoders: interrupt-driven quadrature decoding (no missed steps
 *    however slow loop() is). Needs interrupt-capable pins (any GPIO on
 *    ESP32/ESP8266; D2+D3 on Uno/Nano). Falls back to polling otherwise.
 *  - RFID (MFRC522 library) and IR (IRremote library) are optional.
 */

#ifndef INTEGRALL_INPUT_MODULE_H
#define INTEGRALL_INPUT_MODULE_H

#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"

#if INTEGRALL_MODULE_INPUT_ENABLED

#include <Arduino.h>
#include <new>

#if __has_include(<SPI.h>) && __has_include(<MFRC522.h>)
  #include <SPI.h>
  #include <MFRC522.h>
  #define INTEGRALL_RFID_AVAILABLE 1
#else
  #define INTEGRALL_RFID_AVAILABLE 0
#endif

#if __has_include(<IRremote.hpp>)
  #include <IRremote.hpp>
  #define INTEGRALL_IR_AVAILABLE 1
#elif __has_include(<IRremote.h>)
  #include <IRremote.h>
  #define INTEGRALL_IR_AVAILABLE 1
#else
  #define INTEGRALL_IR_AVAILABLE 0
#endif

#if defined(ESP32) || defined(ESP8266)
  #define INTEGRALL_INPUT_ISR_ATTR IRAM_ATTR
#else
  #define INTEGRALL_INPUT_ISR_ATTR
#endif

#ifndef INTEGRALL_MAX_INPUT_PINS
  #define INTEGRALL_MAX_INPUT_PINS 8
#endif

namespace Integrall {

namespace Detail {
/** Quadrature decoder state for up to 2 encoders, updated from interrupts. */
struct EncoderState {
    uint8_t          clk = 0xFF, dt = 0xFF;
    volatile uint8_t prev = 0;
    volatile int32_t count = 0;      // in quarter steps
};
template <uint8_t N>
struct EncoderIsr {
    static EncoderState st;
    static void INTEGRALL_INPUT_ISR_ATTR onChange() {
        // Gray-code transition table: +1 / -1 / 0 (invalid = bounce)
        static const int8_t table[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
        const uint8_t cur = (uint8_t)((digitalRead(st.clk) << 1) | digitalRead(st.dt));
        st.count = st.count + table[(st.prev << 2) | cur];
        st.prev = cur;
    }
};
template <uint8_t N> EncoderState EncoderIsr<N>::st;
} // namespace Detail

class InputModule {
public:
    InputModule() {
        for (uint8_t i = 0; i < INTEGRALL_MAX_INPUT_PINS; i++) {
            _btnPins[i] = 0xFF; _btnState[i] = false; _lastBtnReading[i] = false; _lastDebounceTime[i] = 0;
        }
    }
    ~InputModule() {
        #if INTEGRALL_RFID_AVAILABLE
        if (_rfid) _rfid->~MFRC522();
        #endif
    }

    /** true once per press (debounced). Call every loop. */
    bool buttonPressed(uint8_t pin, bool activeLow = true) {
        int slot = _getButtonSlot(pin, activeLow);
        if (slot < 0) return false;
        const bool raw = (digitalRead(pin) == HIGH);
        const bool pressed = activeLow ? !raw : raw;
        const unsigned long now = millis();
        if (pressed != _lastBtnReading[slot]) { _lastDebounceTime[slot] = now; _lastBtnReading[slot] = pressed; }
        if ((now - _lastDebounceTime[slot]) >= DEBOUNCE_MS && pressed != _btnState[slot]) {
            _btnState[slot] = pressed;
            return pressed;
        }
        return false;
    }

    /** true while held (debounced). */
    bool buttonHeld(uint8_t pin, bool activeLow = true) {
        int slot = _getButtonSlot(pin, activeLow);
        if (slot < 0) return false;
        buttonPressed(pin, activeLow);       // update debounce state
        return _btnState[slot];
    }

    /** Joystick axis normalised to -100..100 with a dead-zone around centre. */
    int readJoystickAxis(uint8_t pin, int centerValue = 512, int deadzone = 50) {
        int val = analogRead(pin);
        #if defined(ESP32)
        if (centerValue <= 1023) val = (int)map(val, 0, 4095, 0, 1023);
        const int maxVal = (centerValue <= 1023) ? 1023 : 4095;
        #else
        const int maxVal = 1023;
        #endif
        const int diff = val - centerValue;
        if (abs(diff) < deadzone) return 0;
        long mapped = (diff > 0) ? map(val, centerValue + deadzone, maxVal, 0, 100)
                                 : map(val, 0, centerValue - deadzone, -100, 0);
        return (int)constrain(mapped, -100L, 100L);
    }

    /** TTP223-style capacitive touch module (output HIGH when touched). */
    bool touchActive(uint8_t pin) {
        pinMode(pin, INPUT);
        return digitalRead(pin) == HIGH;
    }

    /**
     * Rotary encoder steps since the last call (+ clockwise, - counter-clockwise).
     * The first call attaches interrupts to CLK and DT.
     */
    int readEncoder(uint8_t pinCLK, uint8_t pinDT) {
        int e = _encoderSlot(pinCLK, pinDT);
        if (e < 0) return _readEncoderPolled(pinCLK, pinDT);
        Detail::EncoderState& st = (e == 0) ? Detail::EncoderIsr<0>::st : Detail::EncoderIsr<1>::st;
        noInterrupts();
        const int32_t q = st.count;
        interrupts();
        const int32_t steps = q / 4 - _enc_reported[e];     // 4 quarter-steps per detent
        _enc_reported[e] += steps;
        return (int)steps;
    }

#if INTEGRALL_RFID_AVAILABLE
    /** UID of a newly presented card as hex ("" if none). */
    String readRFID(uint8_t ss = 5, uint8_t rst = 0) {
        if (!_rfid || _rfidSS != ss || _rfidRST != rst) {
            if (_rfid) _rfid->~MFRC522();
            SPI.begin();
            _rfid = new (_rfidStorage) MFRC522(ss, rst);
            _rfid->PCD_Init();
            _rfidSS = ss; _rfidRST = rst;
        }
        if (!_rfid->PICC_IsNewCardPresent() || !_rfid->PICC_ReadCardSerial()) return String();
        String uid;
        for (byte i = 0; i < _rfid->uid.size; i++) {
            if (_rfid->uid.uidByte[i] < 0x10) uid += "0";
            uid += String(_rfid->uid.uidByte[i], HEX);
        }
        _rfid->PICC_HaltA();
        return uid;
    }
#endif

#if INTEGRALL_IR_AVAILABLE
    /** Last decoded IR code (0 if none). IRremote v3+ API. */
    uint32_t readIR(uint8_t pin) {
        if (!_irInited || _irPin != pin) {
            IrReceiver.begin(pin, DISABLE_LED_FEEDBACK);
            _irInited = true; _irPin = pin;
        }
        if (IrReceiver.decode()) {
            uint32_t v = IrReceiver.decodedIRData.decodedRawData;
            IrReceiver.resume();
            return v;
        }
        return 0;
    }
#endif

private:
    static const unsigned long DEBOUNCE_MS = 50;
    uint8_t       _btnPins[INTEGRALL_MAX_INPUT_PINS];
    bool          _btnState[INTEGRALL_MAX_INPUT_PINS];
    bool          _lastBtnReading[INTEGRALL_MAX_INPUT_PINS];
    unsigned long _lastDebounceTime[INTEGRALL_MAX_INPUT_PINS];

    int32_t _enc_reported[2] = {0, 0};
    int     _poll_last_clk = -1;

#if INTEGRALL_RFID_AVAILABLE
    MFRC522* _rfid = nullptr;
    alignas(MFRC522) uint8_t _rfidStorage[sizeof(MFRC522)];
    uint8_t _rfidSS = 0xFF, _rfidRST = 0xFF;
#endif
#if INTEGRALL_IR_AVAILABLE
    bool    _irInited = false;
    uint8_t _irPin = 0xFF;
#endif

    int _getButtonSlot(uint8_t pin, bool activeLow) {
        for (uint8_t i = 0; i < INTEGRALL_MAX_INPUT_PINS; i++) if (_btnPins[i] == pin) return i;
        for (uint8_t i = 0; i < INTEGRALL_MAX_INPUT_PINS; i++) {
            if (_btnPins[i] != 0xFF) continue;
            _btnPins[i] = pin;
            pinMode(pin, activeLow ? INPUT_PULLUP : INPUT);
            const bool raw = (digitalRead(pin) == HIGH);
            _btnState[i] = _lastBtnReading[i] = activeLow ? !raw : raw;
            _lastDebounceTime[i] = millis();
            return i;
        }
        return -1;
    }

    int _encoderSlot(uint8_t clk, uint8_t dt) {
        Detail::EncoderState* s[2] = {&Detail::EncoderIsr<0>::st, &Detail::EncoderIsr<1>::st};
        for (uint8_t i = 0; i < 2; i++) if (s[i]->clk == clk && s[i]->dt == dt) return i;
        #if defined(NOT_AN_INTERRUPT)
        if (digitalPinToInterrupt(clk) == NOT_AN_INTERRUPT || digitalPinToInterrupt(dt) == NOT_AN_INTERRUPT) return -1;
        #endif
        for (uint8_t i = 0; i < 2; i++) {
            if (s[i]->clk != 0xFF) continue;
            s[i]->clk = clk; s[i]->dt = dt;
            pinMode(clk, INPUT_PULLUP); pinMode(dt, INPUT_PULLUP);
            s[i]->prev = (uint8_t)((digitalRead(clk) << 1) | digitalRead(dt));
            void (*isr)() = (i == 0) ? Detail::EncoderIsr<0>::onChange : Detail::EncoderIsr<1>::onChange;
            attachInterrupt(digitalPinToInterrupt(clk), isr, CHANGE);
            attachInterrupt(digitalPinToInterrupt(dt), isr, CHANGE);
            return i;
        }
        return -1;
    }

    int _readEncoderPolled(uint8_t clk, uint8_t dt) {
        pinMode(clk, INPUT_PULLUP); pinMode(dt, INPUT_PULLUP);
        const int c = digitalRead(clk);
        int dir = 0;
        if (_poll_last_clk != -1 && c != _poll_last_clk && c == LOW) dir = (digitalRead(dt) != c) ? 1 : -1;
        _poll_last_clk = c;
        return dir;
    }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_INPUT_ENABLED
#endif // INTEGRALL_INPUT_MODULE_H
