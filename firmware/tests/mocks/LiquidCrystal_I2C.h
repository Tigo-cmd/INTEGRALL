// Fake LCD: keeps a character grid so tests can assert what is on screen,
// and counts writes to prove only changed rows are sent.
#pragma once
#include "Arduino.h"
class LiquidCrystal_I2C {
public:
    static inline int instances = 0;
    char grid[4][21];
    uint8_t cols, rows, cx = 0, cy = 0;
    int charWrites = 0;
    LiquidCrystal_I2C(uint8_t, uint8_t c, uint8_t r) : cols(c), rows(r) { clear(); instances++; }
    ~LiquidCrystal_I2C() { instances--; }
    void init() {}
    void backlight() {}
    void noBacklight() {}
    void clear() { for (auto& row : grid) { memset(row, ' ', 20); row[20] = 0; } cx = cy = 0; }
    void setCursor(uint8_t c, uint8_t r) { cx = c; cy = r; }
    size_t write(uint8_t ch) { if (cy < 4 && cx < 20) grid[cy][cx++] = (char)ch; charWrites++; return 1; }
    void cursor() {} void noCursor() {} void blink() {} void noBlink() {}
    void createChar(uint8_t, uint8_t*) {}
    std::string row(uint8_t r) const { return std::string(grid[r], cols); }
};
