/**
 * CellularModule.h
 *
 * SIM800L / SIM800C / SIM900 GSM modem driver for the Integrall Framework.
 *
 * Fully non-blocking: commands go into a queue and an AT state machine in
 * handle() sends them, matches the modem's replies and reports the outcome
 * through callbacks. Nothing waits with delay().
 *
 *   integrall.cellularBegin(16, 17);                 // ESP32 UART2 RX,TX
 *   integrall.onSMSResult([](bool ok){ ... });       // real delivery-to-network result
 *   integrall.sendSMS("+2348000000000", "Fall detected!");
 *
 * Network registration and incoming calls arrive as unsolicited codes
 * (+CREG, RING/+CLIP), so the module is never polled for them.
 *
 * Power: SIM800L needs 3.7-4.2 V and 2 A bursts. Powering it from a 3.3 V or
 * USB 5 V pin causes resets that look like "random" failures.
 */

#ifndef INTEGRALL_CELLULAR_MODULE_H
#define INTEGRALL_CELLULAR_MODULE_H

#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"

#if INTEGRALL_MODULE_CELLULAR_ENABLED

#include <Arduino.h>
#include <new>
#if !defined(ESP32)
  #include <SoftwareSerial.h>
#endif

#ifndef INTEGRALL_CELL_SMS_QUEUE
  #if defined(__AVR__)
    #define INTEGRALL_CELL_SMS_QUEUE 1
  #else
    #define INTEGRALL_CELL_SMS_QUEUE 4
  #endif
#endif
#ifndef INTEGRALL_CELL_SMS_MAX
  #define INTEGRALL_CELL_SMS_MAX 160
#endif

namespace Integrall {

typedef void (*SmsResultCallback)(bool ok);
typedef void (*IncomingCallCallback)(const char* number);

class CellularModule {
public:
    enum class State : uint8_t { OFF, BOOTING, INIT, READY, WAIT, SMS_MODE, SMS_NUMBER, SMS_BODY, ERROR };

    CellularModule() {}
    ~CellularModule() {
        #if !defined(ESP32)
        if (_soft) { _soft->~SoftwareSerial(); _soft = nullptr; }
        #endif
    }

    /**
     * Start the modem link. Returns immediately; the modem is initialised in
     * the background (ready ~3-10 s later, see isReady()).
     * ESP32: hardware UART2 on the given pins. Others: SoftwareSerial.
     */
    bool begin(uint8_t rxPin = 16, uint8_t txPin = 17, uint32_t baud = 9600) {
        #if defined(ESP32)
        _hw.begin(baud, SERIAL_8N1, rxPin, txPin);
        _port = &_hw;
        #else
        if (_soft) _soft->~SoftwareSerial();
        _soft = new (_soft_storage) SoftwareSerial(rxPin, txPin);
        _soft->begin(baud);
        _port = _soft;
        #endif
        _start();
        return true;
    }

    /** Use any already-started Stream (e.g. Serial1) instead. */
    bool begin(Stream& port) { _port = &port; _start(); return true; }

    // ---- status --------------------------------------------------------------
    State state() const       { return _state; }
    bool  isReady() const     { return _state == State::READY; }
    bool  isRegistered() const{ return _creg == 1 || _creg == 5; }   // home / roaming
    /** Signal quality 0-31 (99 = unknown). Refresh with refreshSignal(). */
    int   signal() const      { return _csq; }
    void  refreshSignal()     { _want_csq = true; }

    void onSMSResult(SmsResultCallback cb)       { _sms_cb = cb; }
    void onIncomingCall(IncomingCallCallback cb) { _call_cb = cb; }

    // ---- actions -------------------------------------------------------------
    /**
     * Queue an SMS. @return false if the queue is full or text is too long.
     * The real outcome comes later via onSMSResult().
     */
    bool sendSMS(const char* number, const char* message) {
        if (!_port || !number || !message || strlen(message) > INTEGRALL_CELL_SMS_MAX || strlen(number) >= sizeof(Sms::number)) {
            return false;
        }
        if (_sms_count >= INTEGRALL_CELL_SMS_QUEUE) {
            INTEGRALL_LOG_WARN("Cellular: SMS queue full");
            return false;
        }
        Sms& s = _sms[(_sms_head + _sms_count) % INTEGRALL_CELL_SMS_QUEUE];
        strncpy(s.number, number, sizeof(s.number) - 1); s.number[sizeof(s.number) - 1] = '\0';
        strncpy(s.text, message, sizeof(s.text) - 1);     s.text[sizeof(s.text) - 1] = '\0';
        _sms_count++;
        return true;
    }

    /** Dial a voice call (ATD). Returns false if busy. */
    bool call(const char* number) {
        if (!isReady() || !number || strlen(number) >= sizeof(_dial)) return false;
        strncpy(_dial, number, sizeof(_dial) - 1); _dial[sizeof(_dial) - 1] = '\0';
        _want_dial = true;
        return true;
    }

    /** Hang up any call. */
    void hangup() { _want_hangup = true; }

    /** Called by System::handle(). Never blocks. */
    void handle() {
        if (!_port) return;
        _readModem();
        const unsigned long now = millis();

        switch (_state) {
            case State::OFF:
            case State::ERROR:
                if ((long)(now - _retry_at) >= 0) _start();
                break;

            case State::BOOTING:                       // modem needs a few s after power-up
                if ((long)(now - _retry_at) >= 0) _initStep(0);
                break;

            case State::INIT:
                if (_reply != Reply::NONE) {
                    if (_reply == Reply::OK) _initStep(_init_step + 1);
                    else { _retry_at = now + 2000; _state = State::BOOTING; }   // retry from AT
                } else if (now - _sent_at > 2000) {
                    _retry_at = now + 2000; _state = State::BOOTING;
                }
                break;

            case State::READY:
                if (_want_hangup) { _want_hangup = false; _send("ATH"); _state = State::WAIT; break; }
                if (_want_dial)   { _want_dial = false; _port->print("ATD"); _port->print(_dial); _send(";"); _state = State::WAIT; break; }
                if (_want_csq)    { _want_csq = false; _send("AT+CSQ"); _state = State::WAIT; break; }
                if (_sms_count > 0) { _send("AT+CMGF=1"); _state = State::SMS_MODE; }
                break;

            case State::WAIT:                          // generic "send, await OK/ERROR"
                if (_reply != Reply::NONE || now - _sent_at > 5000) _state = State::READY;
                break;

            case State::SMS_MODE:
                if (_reply == Reply::OK) {
                    _port->print("AT+CMGS=\""); _port->print(_sms[_sms_head].number); _send("\"");
                    _state = State::SMS_NUMBER;
                } else if (_reply != Reply::NONE || now - _sent_at > 5000) _smsDone(false);
                break;

            case State::SMS_NUMBER:                    // wait for the '>' prompt
                if (_reply == Reply::PROMPT) {
                    _port->print(_sms[_sms_head].text);
                    _port->write((uint8_t)26);         // Ctrl-Z sends
                    _sent_at = now; _reply = Reply::NONE;
                    _state = State::SMS_BODY;
                } else if (_reply != Reply::NONE || now - _sent_at > 10000) {
                    _port->write((uint8_t)27);         // ESC aborts a half-open prompt
                    _smsDone(false);
                }
                break;

            case State::SMS_BODY:                      // network ack can take up to 60 s
                if (_reply == Reply::OK) _smsDone(_got_cmgs);
                else if (_reply != Reply::NONE || now - _sent_at > 60000) _smsDone(false);
                break;
        }
    }

private:
    enum class Reply : uint8_t { NONE, OK, ERROR, PROMPT };
    struct Sms { char number[20]; char text[INTEGRALL_CELL_SMS_MAX + 1]; };

    Stream*       _port = nullptr;
    #if defined(ESP32)
    HardwareSerial _hw{2};
    #else
    SoftwareSerial* _soft = nullptr;
    alignas(SoftwareSerial) uint8_t _soft_storage[sizeof(SoftwareSerial)];
    #endif

    State         _state = State::OFF;
    Reply         _reply = Reply::NONE;
    uint8_t       _init_step = 0;
    unsigned long _sent_at = 0;
    unsigned long _retry_at = 0;
    char          _line[64];
    uint8_t       _line_len = 0;
    int           _csq = 99;
    int           _creg = 0;
    bool          _got_cmgs = false;
    bool          _want_csq = false, _want_dial = false, _want_hangup = false;
    char          _dial[20] = {0};

    Sms     _sms[INTEGRALL_CELL_SMS_QUEUE];
    uint8_t _sms_head = 0, _sms_count = 0;

    SmsResultCallback    _sms_cb = nullptr;
    IncomingCallCallback _call_cb = nullptr;

    void _start() {
        _state = State::BOOTING;
        _retry_at = millis() + 3000;                  // let the modem boot
        _line_len = 0;
        _reply = Reply::NONE;
    }

    void _send(const char* tail) {
        _reply = Reply::NONE;
        _got_cmgs = false;
        _port->print(tail);
        _port->print("\r");
        _sent_at = millis();
    }

    /** Init sequence: AT, echo off, text mode, registration URCs, caller ID. */
    void _initStep(uint8_t step) {
        static const char* const cmds[] = {"AT", "ATE0", "AT+CMGF=1", "AT+CREG=1", "AT+CLIP=1", "AT+CREG?", "AT+CSQ"};
        const uint8_t n = sizeof(cmds) / sizeof(cmds[0]);
        _init_step = step;
        if (step >= n) {
            _state = State::READY;
            INTEGRALL_LOG_INFO("Cellular: modem ready");
            return;
        }
        _state = State::INIT;
        _send(cmds[step]);
    }

    void _smsDone(bool ok) {
        if (ok) { INTEGRALL_LOG_INFO_VAL("Cellular: SMS sent to ", _sms[_sms_head].number); }
        else    { INTEGRALL_LOG_WARN_VAL("Cellular: SMS FAILED to ", _sms[_sms_head].number); }
        _sms_head = (_sms_head + 1) % INTEGRALL_CELL_SMS_QUEUE;
        _sms_count--;
        _state = State::READY;
        if (_sms_cb) _sms_cb(ok);
    }

    void _readModem() {
        int budget = 128;
        while (budget-- > 0 && _port->available() > 0) {
            char c = (char)_port->read();
            if (c == '>' && _line_len == 0) { _reply = Reply::PROMPT; continue; }
            if (c == '\r') continue;
            if (c == '\n') {
                _line[_line_len] = '\0';
                if (_line_len) _onLine(_line);
                _line_len = 0;
                continue;
            }
            if (_line_len < sizeof(_line) - 1) _line[_line_len++] = c;
        }
    }

    void _onLine(const char* l) {
        #if INTEGRALL_DEBUG_LEVEL >= 4
        Serial.print(F("[GSM] ")); Serial.println(l);
        #endif
        if (strcmp(l, "OK") == 0) { _reply = Reply::OK; return; }
        if (strcmp(l, "ERROR") == 0 || strncmp(l, "+CMS ERROR", 10) == 0 || strncmp(l, "+CME ERROR", 10) == 0
            || strcmp(l, "NO CARRIER") == 0 || strcmp(l, "BUSY") == 0 || strcmp(l, "NO ANSWER") == 0) {
            _reply = Reply::ERROR; return;
        }
        if (strncmp(l, "+CMGS:", 6) == 0) { _got_cmgs = true; return; }
        if (strncmp(l, "+CSQ:", 5) == 0) { _csq = atoi(l + 5); return; }
        if (strncmp(l, "+CREG:", 6) == 0) {
            // "+CREG: <n>,<stat>" (query reply) or "+CREG: <stat>" (URC)
            const char* comma = strchr(l, ',');
            _creg = atoi(comma ? comma + 1 : l + 6);
            return;
        }
        if (strncmp(l, "+CLIP:", 6) == 0 && _call_cb) {
            const char* q1 = strchr(l, '"');
            const char* q2 = q1 ? strchr(q1 + 1, '"') : nullptr;
            char num[20] = {0};
            if (q1 && q2 && (size_t)(q2 - q1 - 1) < sizeof(num)) memcpy(num, q1 + 1, q2 - q1 - 1);
            _call_cb(num);
            return;
        }
        if (strcmp(l, "NORMAL POWER DOWN") == 0 || strncmp(l, "UNDER-VOLTAGE", 13) == 0) {
            INTEGRALL_LOG_ERROR("Cellular: modem power problem (needs 3.7-4.2 V, 2 A)");
            _state = State::ERROR;
            _retry_at = millis() + 5000;
        }
    }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_CELLULAR_ENABLED
#endif // INTEGRALL_CELLULAR_MODULE_H
