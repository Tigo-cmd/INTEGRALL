/**
 * Util.h — small shared helpers used across Integrall modules.
 */
#ifndef INTEGRALL_UTIL_H
#define INTEGRALL_UTIL_H

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>

#if __has_include(<Wire.h>)
  #include <Wire.h>
  #define INTEGRALL_HAS_WIRE 1
#else
  #define INTEGRALL_HAS_WIRE 0
#endif

namespace Integrall {
namespace Util {

/**
 * Format a float portably. AVR's printf has no %f support (prints "?"), so
 * never use "%f" in Integrall code — use this instead.
 * Writes "nan" for NAN. Returns buf.
 */
inline char* fmtFloat(char* buf, size_t len, float value, uint8_t decimals = 1) {
    if (!buf || len == 0) return buf;
    if (isnan(value) || isinf(value)) {
        strncpy(buf, isnan(value) ? "nan" : "inf", len - 1);
        buf[len - 1] = '\0';
        return buf;
    }
#if defined(__AVR__)
    char tmp[24];
    dtostrf(value, 1, decimals, tmp);
    strncpy(buf, tmp, len - 1);
    buf[len - 1] = '\0';
#else
    snprintf(buf, len, "%.*f", (int)decimals, (double)value);
#endif
    return buf;
}

/** Shared I2C bus state. Pins of -1 mean "board default SDA/SCL". */
struct I2CState { int sda = -1; int scl = -1; bool started = false; };
inline I2CState& i2cState() { static I2CState s; return s; }

/**
 * Choose I2C pins (ESP32/ESP8266 only). Must be called before anything uses
 * the bus — i.e. before integrall.begin() (System::setI2CPins does this).
 */
inline void i2cSetPins(int sda, int scl) { i2cState().sda = sda; i2cState().scl = scl; }

/** Initialise the shared I2C bus once (safe to call repeatedly). */
inline void i2cBegin() {
#if INTEGRALL_HAS_WIRE
    I2CState& st = i2cState();
    if (st.started) return;
    #if defined(ESP32) || defined(ESP8266)
    if (st.sda >= 0 && st.scl >= 0) Wire.begin(st.sda, st.scl);
    else Wire.begin();
    #else
    Wire.begin();
    #endif
    #if defined(ESP32)
    Wire.setTimeOut(20);                 // a stuck bus can't freeze loop() for long
    #elif defined(WIRE_HAS_TIMEOUT)
    Wire.setWireTimeout(25000, true);    // AVR: 25 ms, auto-reset the bus on lockup
    #endif
    st.started = true;
#endif
}

/** true if a device ACKs at this 7-bit I2C address. Takes ~0.1-1 ms. */
inline bool i2cPresent(uint8_t addr) {
#if INTEGRALL_HAS_WIRE
    i2cBegin();
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
#else
    (void)addr;
    return false;
#endif
}

/** Simple retry back-off so missing parts are not re-probed every loop. */
struct Backoff {
    unsigned long next_ms = 0;
    uint32_t      step_ms = 1000;
    bool ready() const { return (long)(millis() - next_ms) >= 0; }
    void fail()  { next_ms = millis() + step_ms; if (step_ms < 30000UL) step_ms *= 2; }
    void reset() { step_ms = 1000; next_ms = 0; }
};

} // namespace Util
} // namespace Integrall

#endif // INTEGRALL_UTIL_H
