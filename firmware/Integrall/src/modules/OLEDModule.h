/**
 * OLEDModule.h
 * 
 * Simple OLED display wrapper for Integrall Framework.
 * Hides SSD1306 initialization, I2C address, and Adafruit library complexity.
 *
 * Compatible with: 0.96" and 1.3" I2C OLED (SSD1306 driver).
 */

#ifndef INTEGRALL_OLED_MODULE_H
#define INTEGRALL_OLED_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_OLED_ENABLED

#include <Wire.h>

  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>

#ifndef INTEGRALL_OLED_WIDTH
  #define INTEGRALL_OLED_WIDTH  128
#endif
#ifndef INTEGRALL_OLED_HEIGHT
  #define INTEGRALL_OLED_HEIGHT 64
#endif

namespace Integrall {

class OLEDModule {
public:
    OLEDModule() : _display(nullptr), _initialized(false) {}

    /**
     * Initialize the OLED display
     * @param address I2C address, typically 0x3C or 0x3D
     */
    bool begin(uint8_t address = 0x3C) {
        Wire.begin();
        _display = new Adafruit_SSD1306(INTEGRALL_OLED_WIDTH, INTEGRALL_OLED_HEIGHT, &Wire, -1);
        if (!_display->begin(SSD1306_SWITCHCAPVCC, address)) {
            return false;
        }
        _display->clearDisplay();
        _display->setTextColor(SSD1306_WHITE);
        _display->setTextSize(1);
        _display->display();
        _initialized = true;
        return true;
    }

    /**
     * Print text at a position. Auto-displays after print.
     * @param text   Text to show
     * @param col    Character column (each char is 6px wide)
     * @param row    Character row (each row is 8px tall)
     * @param clear  Clear screen before printing (default: false)
     */
    void print(const char* text, uint8_t col = 0, uint8_t row = 0, bool clear = false) {
        if (!_initialized) return;
        if (clear) _display->clearDisplay();
        _display->setCursor(col * 6, row * 8);
        _display->print(text);
        _display->display();
    }

    /**
     * Print a number value on the OLED
     */
    void printValue(const char* label, float value, uint8_t row = 0) {
        if (!_initialized) return;
        char buf[24];
        snprintf(buf, sizeof(buf), "%s%.1f", label, value);
        print(buf, 0, row);
    }

    /**
     * Draw a simple progress bar (useful for signal strength / battery)
     * @param percent Value from 0 to 100
     * @param row     Pixel row to draw bar at
     */
    void drawBar(uint8_t percent, uint8_t y_pixel = 56) {
        if (!_initialized) return;
        uint8_t barWidth = map(percent, 0, 100, 0, INTEGRALL_OLED_WIDTH - 4);
        _display->drawRect(0, y_pixel, INTEGRALL_OLED_WIDTH - 2, 7, SSD1306_WHITE);
        _display->fillRect(1, y_pixel + 1, barWidth, 5, SSD1306_WHITE);
        _display->display();
    }

    /**
     * Clear the display
     */
    void clear() {
        if (!_initialized) return;
        _display->clearDisplay();
        _display->display();
    }

    // Access raw Adafruit object for advanced use
    Adafruit_SSD1306* getRaw() { return _display; }

private:
    Adafruit_SSD1306* _display;
    bool _initialized;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_OLED_ENABLED
#endif // INTEGRALL_OLED_MODULE_H
