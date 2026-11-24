// Fake I2C bus: tests choose which addresses ACK and what bytes they return.
#pragma once
#include "Arduino.h"
#include <set>
#include <deque>
class TwoWire {
public:
    std::set<uint8_t> present;
    std::deque<uint8_t> replies;
    std::vector<uint8_t> written;
    uint8_t cur = 0;
    void begin() {}
    void begin(int, int) {}
    void setClock(uint32_t) {}
    void beginTransmission(uint8_t a) { cur = a; }
    size_t write(uint8_t b) { written.push_back(b); return 1; }
    uint8_t endTransmission(bool = true) { return present.count(cur) ? 0 : 2; }
    uint8_t requestFrom(uint8_t a, uint8_t n) {
        if (!present.count(a) || replies.size() < n) return 0;
        return n;
    }
    int read() { if (replies.empty()) return -1; uint8_t v = replies.front(); replies.pop_front(); return v; }
    int available() { return (int)replies.size(); }
};
inline TwoWire Wire;
