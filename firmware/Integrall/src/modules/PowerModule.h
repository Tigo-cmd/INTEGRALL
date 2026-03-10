/**
 * PowerModule.h
 * 
 * Logic for energy-aware IoT devices. 
 * Supports current/voltage measuring (INA219), battery health monitoring, 
 * and deep-sleep optimization for solar/battery-powered projects.
 */

#ifndef INTEGRALL_POWER_MODULE_H
#define INTEGRALL_POWER_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

// Optional: include INA219 support if user has the library installed
// Install via Library Manager: "Adafruit INA219"
#if __has_include(<Adafruit_INA219.h>)
  #include <Adafruit_INA219.h>
  #define INTEGRALL_INA219_AVAILABLE 1
#else
  #define INTEGRALL_INA219_AVAILABLE 0
#endif

namespace Integrall {

class PowerModule {
public:
    PowerModule() : _inaStarted(false) {}

    /**
     * Start the INA219 current/voltage sensor (I2C)
     */
#if INTEGRALL_INA219_AVAILABLE
    bool beginINA(uint8_t addr = 0x40) {
        if (!_ina.begin(addr)) return false;
        _inaStarted = true;
        return true;
    }

    /**
     * Get bus voltage (V)
     */
    float getVoltage() {
        if (!_inaStarted) return -1.0f;
        return _ina.getBusVoltage_V();
    }

    /**
     * Get current consumption (mA)
     */
    float getCurrent() {
        if (!_inaStarted) return -1.0f;
        return _ina.getCurrent_mA();
    }

    /**
     * Get power consumption (mW)
     */
    float getPower() {
        if (!_inaStarted) return -1.0f;
        return _ina.getPower_mW();
    }
#endif

    /**
     * Simple battery percentage estimation based on Li-Po voltage curve (3.0V-4.2V)
     * @param voltage Current measured voltage
     */
    int getBatteryPercent(float voltage) {
        if (voltage < 3.0f) return 0;
        if (voltage > 4.2f) return 100;
        return map(voltage * 100, 300, 420, 0, 100);
    }

    /**
     * Go to Deep Sleep for specified duration (ESP32 only)
     * @param seconds Duration in seconds
     */
    void deepSleep(uint32_t seconds) {
        #if defined(ESP32)
        esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
        esp_deep_sleep_start();
        #elif defined(ESP8266)
        ESP.deepSleep(seconds * 1000000ULL);
        #else
        delay(seconds * 1000); // Fail-safe for non-sleep boards
        #endif
    }

private:
#if INTEGRALL_INA219_AVAILABLE
    Adafruit_INA219 _ina;
#endif
    bool _inaStarted;
};

} // namespace Integrall

#endif // INTEGRALL_POWER_MODULE_H
