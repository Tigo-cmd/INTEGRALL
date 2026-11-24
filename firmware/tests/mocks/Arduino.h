// Minimal fake Arduino core for host unit tests. Deterministic: time only
// moves when a test calls Fake::advance(); pins are plain arrays.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

typedef uint8_t byte;
typedef bool boolean;

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define CHANGE 1
#define RISING 3
#define FALLING 2
#define PI 3.14159265358979f
#define HEX 16
#define DEC 10
#define NOT_AN_INTERRUPT -1
#define digitalPinToInterrupt(p) ((int)(p))
#define A0 14
#ifndef isnan
using std::isnan;
using std::isinf;
#endif

template <typename T, typename L, typename H>
inline T constrain(T x, L lo, H hi) { return x < (T)lo ? (T)lo : (x > (T)hi ? (T)hi : x); }
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
inline bool isDigit(char c) { return c >= '0' && c <= '9'; }

// ---------------------------------------------------------------- fake board
namespace Fake {
struct Board {
    unsigned long us = 0;
    int pinLevel[64] = {0};     // what digitalRead returns / last digitalWrite
    int pinMode[64] = {0};
    int analog[64] = {0};
    std::vector<int> writes;    // log of (pin<<1 | level)
    void (*isr[64])() = {nullptr};
    int toneFreq[64] = {0};
};
inline Board& b() { static Board s; return s; }
inline void reset() { b() = Board(); }
inline void advanceMs(unsigned long ms) { b().us += ms * 1000UL; }
inline void advanceUs(unsigned long us) { b().us += us; }
/** Drive an input pin and fire its interrupt like real hardware would. */
inline void setPin(int pin, int level) {
    bool changed = b().pinLevel[pin] != level;
    b().pinLevel[pin] = level;
    if (changed && b().isr[pin]) b().isr[pin]();
}
}

inline unsigned long millis() { return Fake::b().us / 1000UL; }
inline unsigned long micros() { return Fake::b().us; }
inline void delay(unsigned long ms) { Fake::advanceMs(ms); }
inline void delayMicroseconds(unsigned int us) { Fake::advanceUs(us); }
inline void yield() {}
inline void pinMode(uint8_t p, uint8_t m) {
    Fake::b().pinMode[p] = m;
    if (m == INPUT_PULLUP && Fake::b().pinLevel[p] == 0 && Fake::b().writes.empty()) Fake::b().pinLevel[p] = HIGH;
}
inline void digitalWrite(uint8_t p, uint8_t v) { Fake::b().pinLevel[p] = v ? 1 : 0; Fake::b().writes.push_back((p << 1) | (v ? 1 : 0)); }
inline int digitalRead(uint8_t p) { return Fake::b().pinLevel[p]; }
inline int analogRead(uint8_t p) { return Fake::b().analog[p]; }
inline void analogWrite(uint8_t p, int v) { Fake::b().analog[p] = v; }
inline void attachInterrupt(int irq, void (*f)(), int) { Fake::b().isr[irq] = f; }
inline void detachInterrupt(int irq) { Fake::b().isr[irq] = nullptr; }
inline void noInterrupts() {}
inline void interrupts() {}
inline void tone(uint8_t p, unsigned int f, unsigned long = 0) { Fake::b().toneFreq[p] = (int)f; }
inline void noTone(uint8_t p) { Fake::b().toneFreq[p] = 0; }
inline unsigned long pulseIn(uint8_t, uint8_t, unsigned long = 1000000UL) { return 0; }

// ---------------------------------------------------------------- strings
class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper*>(s))

class String {
public:
    String() {}
    String(const char* s) : _s(s ? s : "") {}
    String(const std::string& s) : _s(s) {}
    String(const __FlashStringHelper* f) : _s(reinterpret_cast<const char*>(f)) {}
    String(char c) : _s(1, c) {}
    String(int v, int base = 10) { char b[34]; if (base == 16) snprintf(b, sizeof b, "%x", v); else snprintf(b, sizeof b, "%d", v); _s = b; }
    String(unsigned v, int base = 10) { char b[34]; snprintf(b, sizeof b, base == 16 ? "%x" : "%u", v); _s = b; }
    String(long v) { _s = std::to_string(v); }
    String(unsigned long v) { _s = std::to_string(v); }
    String(float v, int d = 2) { char b[32]; snprintf(b, sizeof b, "%.*f", d, v); _s = b; }
    const char* c_str() const { return _s.c_str(); }
    size_t length() const { return _s.size(); }
    void reserve(size_t n) { _s.reserve(n); }
    char operator[](size_t i) const { return i < _s.size() ? _s[i] : 0; }
    String& operator+=(const String& o) { _s += o._s; return *this; }
    String& operator+=(const char* o) { _s += o; return *this; }
    String& operator+=(char c) { _s += c; return *this; }
    String& operator+=(int v) { _s += std::to_string(v); return *this; }
    String& operator+=(unsigned v) { _s += std::to_string(v); return *this; }
    String& operator+=(unsigned long v) { _s += std::to_string(v); return *this; }
    String& operator+=(const __FlashStringHelper* f) { _s += reinterpret_cast<const char*>(f); return *this; }
    friend String operator+(const String& a, const String& b) { return String(a._s + b._s); }
    friend String operator+(const String& a, const char* b) { return String(a._s + b); }
    friend String operator+(const char* a, const String& b) { return String(std::string(a) + b._s); }
    bool operator==(const String& o) const { return _s == o._s; }
    bool operator==(const char* o) const { return _s == o; }
    bool operator!=(const String& o) const { return _s != o._s; }
    int indexOf(char c, int from = 0) const { auto p = _s.find(c, from); return p == std::string::npos ? -1 : (int)p; }
    int indexOf(const char* s, int from = 0) const { auto p = _s.find(s, from); return p == std::string::npos ? -1 : (int)p; }
    int lastIndexOf(char c) const { auto p = _s.rfind(c); return p == std::string::npos ? -1 : (int)p; }
    String substring(int a, int b = -1) const { if (b < 0 || b > (int)_s.size()) b = _s.size(); return a >= b ? String() : String(_s.substr(a, b - a)); }
    bool startsWith(const char* p) const { return _s.rfind(p, 0) == 0; }
    bool startsWith(const String& p) const { return startsWith(p.c_str()); }
    bool endsWith(const char* p) const { size_t n = strlen(p); return _s.size() >= n && _s.compare(_s.size() - n, n, p) == 0; }
    bool equalsIgnoreCase(const String& o) const { return strcasecmp(_s.c_str(), o._s.c_str()) == 0; }
    void trim() { size_t a = _s.find_first_not_of(" \t\r\n"); size_t b = _s.find_last_not_of(" \t\r\n"); _s = (a == std::string::npos) ? "" : _s.substr(a, b - a + 1); }
    long toInt() const { return atol(_s.c_str()); }
    const std::string& std() const { return _s; }
private:
    std::string _s;
};

// ---------------------------------------------------------------- streams
class Print {
public:
    virtual ~Print() {}
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t* b, size_t n) { for (size_t i = 0; i < n; i++) write(b[i]); return n; }
    size_t print(const char* s) { return write((const uint8_t*)s, strlen(s)); }
    size_t print(const String& s) { return print(s.c_str()); }
    size_t print(const __FlashStringHelper* f) { return print(reinterpret_cast<const char*>(f)); }
    size_t print(char c) { return write((uint8_t)c); }
    size_t print(int v) { return print(std::to_string(v).c_str()); }
    size_t print(unsigned v) { return print(std::to_string(v).c_str()); }
    size_t print(long v) { return print(std::to_string(v).c_str()); }
    size_t print(unsigned long v) { return print(std::to_string(v).c_str()); }
    size_t print(double v) { char b[32]; snprintf(b, sizeof b, "%.2f", v); return print(b); }
    template <typename T> size_t println(T v) { size_t n = print(v); return n + print("\r\n"); }
    size_t println() { return print("\r\n"); }
};
class Stream : public Print {
public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() {}
    virtual size_t readBytes(char* b, size_t n) { size_t i = 0; while (i < n && available()) b[i++] = (char)read(); return i; }
};

/** In-memory serial port: `rx` is what the device will read, `tx` what it wrote. */
class FakeSerial : public Stream {
public:
    std::string rx, tx;
    void begin(unsigned long) {}
    explicit operator bool() const { return true; }
    size_t write(uint8_t c) override { tx += (char)c; return 1; }
    int available() override { return (int)rx.size(); }
    int read() override { if (rx.empty()) return -1; int c = (uint8_t)rx[0]; rx.erase(0, 1); return c; }
    int peek() override { return rx.empty() ? -1 : (uint8_t)rx[0]; }
};
extern FakeSerial Serial;
inline FakeSerial Serial;
