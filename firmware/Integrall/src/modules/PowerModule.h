/**
 * PowerModule.h
 *
 * INA219 voltage/current/power monitor, Li-ion battery estimate and deep sleep.
 * INA219 support needs the "Adafruit INA219" library.
 */

#ifndef INTEGRALL_POWER_MODULE_H
#define INTEGRALL_POWER_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"
#include "../core/Util.h"

#if __has_include(<Adafruit_INA219.h>)
  #include <Adafruit_INA219.h>
  #define INTEGRALL_INA219_AVAILABLE 1
#else
  #define INTEGRALL_INA219_AVAILABLE 0
#endif

namespace Integrall {

class PowerModule {
public:
    PowerModule() {}
    ~PowerModule() {
        #if INTEGRALL_INA219_AVAILABLE
        if (_ina) { _ina->~Adafruit_INA219(); _ina = nullptr; }
        #endif
    }

#if INTEGRALL_INA219_AVAILABLE
    /** Start the INA219 at an I2C address (0x40-0x4F). @return false if not found. */
    bool beginINA(uint8_t addr = 0x40) {
        if (!Util::i2cPresent(addr)) {
            INTEGRALL_LOG_WARN_VAL("INA219 not found at I2C 0x", String(addr, HEX));
            return false;
        }
        if (_ina) { _ina->~Adafruit_INA219(); _ina = nullptr; }
        _ina = new (_inaStorage) Adafruit_INA219(addr);   // address goes to the constructor
        if (!_ina->begin(&Wire)) { _ina->~Adafruit_INA219(); _ina = nullptr; return false; }
        return true;
    }
    bool  inaReady() const   { return _ina != nullptr; }
    /** Bus voltage in V, NAN if not started. */
    float getVoltage()       { return _ina ? _ina->getBusVoltage_V() : NAN; }
    /** Current in mA, NAN if not started. */
    float getCurrent()       { return _ina ? _ina->getCurrent_mA() : NAN; }
    /** Power in mW, NAN if not started. */
    float getPower()         { return _ina ? _ina->getPower_mW() : NAN; }
#endif

    /**
     * Li-ion/LiPo state of charge from resting cell voltage (piece-wise
     * linear on a typical discharge curve, more accurate than 3.0-4.2 V linear).
     * @return 0-100, or -1 if the voltage is NAN.
     */
    int getBatteryPercent(float voltage) const {
        if (isnan(voltage)) return -1;
        static const float  v[] = {3.00f, 3.45f, 3.68f, 3.74f, 3.77f, 3.79f, 3.82f, 3.87f, 3.92f, 3.98f, 4.06f, 4.20f};
        static const int8_t p[] = {0,     5,     10,    20,    30,    40,    50,    60,    70,    80,    90,    100};
        if (voltage <= v[0]) return 0;
        if (voltage >= v[11]) return 100;
        for (uint8_t i = 1; i < 12; i++) {
            if (voltage < v[i]) {
                float t = (voltage - v[i - 1]) / (v[i] - v[i - 1]);
                return (int)(p[i - 1] + t * (p[i] - p[i - 1]) + 0.5f);
            }
        }
        return 100;
    }

    /**
     * Deep sleep for N seconds. ESP32: wakes via timer and RESTARTS (setup()
     * runs again). ESP8266: requires GPIO16 wired to RST. Other boards: logs a
     * warning and returns immediately (never blocks).
     */
    void deepSleep(uint32_t seconds) {
        #if defined(ESP32)
        esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
        esp_deep_sleep_start();
        #elif defined(ESP8266)
        ESP.deepSleep((uint64_t)seconds * 1000000ULL);
        #else
        (void)seconds;
        INTEGRALL_LOG_WARN("deepSleep() not supported on this board (AVR: use the LowPower library)");
        #endif
    }

private:
#if INTEGRALL_INA219_AVAILABLE
    Adafruit_INA219* _ina = nullptr;
    alignas(Adafruit_INA219) uint8_t _inaStorage[sizeof(Adafruit_INA219)];
#endif
};

} // namespace Integrall

#endif // INTEGRALL_POWER_MODULE_H
