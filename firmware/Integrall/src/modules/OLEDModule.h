/**
 * OLEDModule.h
 *
 * SSD1306 I2C OLED (128x64 or 128x32) for the Integrall Framework.
 *
 * Drawing calls only update the RAM framebuffer and mark it dirty; handle()
 * pushes it to the panel at most every INTEGRALL_OLED_MIN_FLUSH_MS. Many
 * prints per loop therefore cost one I2C transfer instead of one each
 * (a full 128x64 push is ~25 ms at 400 kHz).
 *
 * Needs "Adafruit SSD1306" + "Adafruit GFX". The 128x64 framebuffer uses
 * 1 KB of RAM — too much for an Uno/Nano alongside other modules.
 */

#ifndef INTEGRALL_OLED_MODULE_H
#define INTEGRALL_OLED_MODULE_H

#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"
#include "../core/Util.h"

#if INTEGRALL_MODULE_OLED_ENABLED

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <new>

#ifndef INTEGRALL_OLED_WIDTH
  #define INTEGRALL_OLED_WIDTH  128
#endif
#ifndef INTEGRALL_OLED_HEIGHT
  #define INTEGRALL_OLED_HEIGHT 64
#endif
#ifndef INTEGRALL_OLED_MIN_FLUSH_MS
  #define INTEGRALL_OLED_MIN_FLUSH_MS 50    // max ~20 frames/s
#endif

namespace Integrall {

class OLEDModule {
public:
    OLEDModule() {}
    ~OLEDModule() { _destroy(); }

    /**
     * Initialise the panel.
     * @param address I2C address (0x3C or 0x3D), or 0 to auto-detect
     * @return false if no panel answers on the bus
     */
    bool begin(uint8_t address = 0) {
        if (address == 0) {
            if (Util::i2cPresent(0x3C)) address = 0x3C;
            else if (Util::i2cPresent(0x3D)) address = 0x3D;
        }
        if (address == 0 || !Util::i2cPresent(address)) {
            INTEGRALL_LOG_WARN("OLED not found on I2C (tried 0x3C/0x3D)");
            _destroy();
            return false;
        }
        if (_display && _address == address) return true;
        _destroy();
        _display = new (_storage) Adafruit_SSD1306(INTEGRALL_OLED_WIDTH, INTEGRALL_OLED_HEIGHT, &Wire, -1);
        if (!_display->begin(SSD1306_SWITCHCAPVCC, address)) {   // fails if framebuffer malloc fails
            INTEGRALL_LOG_ERROR("OLED: not enough RAM for the framebuffer");
            _destroy();
            return false;
        }
        _address = address;
        _display->clearDisplay();
        // Opaque text so re-printing in place erases the old glyphs
        _display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        _display->setTextSize(1);
        _display->display();
        _dirty = false;
        INTEGRALL_LOG_INFO_VAL("OLED ready at I2C 0x", String(address, HEX));
        return true;
    }

    bool isPresent() const { return _display != nullptr; }

    /** Print text at a character cell (6x8 px cells at size 1). */
    void print(const char* text, uint8_t col = 0, uint8_t row = 0, bool clear = false) {
        if (!_display || !text) return;
        if (clear) _display->clearDisplay();
        _display->setCursor(col * 6, row * 8);
        _display->print(text);
        _dirty = true;
    }

    /** Print "label value" on a row, e.g. printValue("Temp: ", 24.5, 2). */
    void printValue(const char* label, float value, uint8_t row = 0, uint8_t decimals = 1) {
        if (!_display || !label) return;
        char num[16];
        Util::fmtFloat(num, sizeof(num), value, decimals);
        char buf[32];
        snprintf(buf, sizeof(buf), "%s%s   ", label, num);
        print(buf, 0, row);
    }

    /** Progress bar 0-100 % at a pixel row. */
    void drawBar(uint8_t percent, uint8_t y_pixel = INTEGRALL_OLED_HEIGHT - 8) {
        if (!_display) return;
        if (percent > 100) percent = 100;
        const int16_t w = INTEGRALL_OLED_WIDTH - 2;
        const int16_t fill = (int16_t)((long)percent * (w - 2) / 100);
        _display->fillRect(0, y_pixel, w, 7, SSD1306_BLACK);
        _display->drawRect(0, y_pixel, w, 7, SSD1306_WHITE);
        _display->fillRect(1, y_pixel + 1, fill, 5, SSD1306_WHITE);
        _dirty = true;
    }

    void clear() {
        if (!_display) return;
        _display->clearDisplay();
        _dirty = true;
    }

    /** Call every loop: pushes the framebuffer if it changed (rate-limited). */
    void handle() {
        if (!_display || !_dirty) return;
        const unsigned long now = millis();
        if (now - _last_flush < INTEGRALL_OLED_MIN_FLUSH_MS) return;
        _last_flush = now;
        _dirty = false;
        _display->display();
    }

    /** Push immediately (e.g. right before deep sleep). */
    void flush() { if (_display) { _display->display(); _dirty = false; _last_flush = millis(); } }

    /**
     * Raw Adafruit driver for custom graphics. After drawing, call
     * markDirty() (or flush()) so the change reaches the panel.
     */
    Adafruit_SSD1306* getRaw() { return _display; }
    void markDirty() { _dirty = true; }

private:
    Adafruit_SSD1306* _display = nullptr;
    alignas(Adafruit_SSD1306) uint8_t _storage[sizeof(Adafruit_SSD1306)];
    uint8_t       _address = 0;
    bool          _dirty = false;
    unsigned long _last_flush = 0;

    void _destroy() { if (_display) { _display->~Adafruit_SSD1306(); _display = nullptr; } }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_OLED_ENABLED
#endif // INTEGRALL_OLED_MODULE_H
