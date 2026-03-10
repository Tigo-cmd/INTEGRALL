/**
 * TimeModule.h
 * 
 * Centralized time manager for the Integrall Framework. 
 * Combines hardware RTC (Real-Time Clock) and network NTP syncing.
 * Essential for scheduled tasks, logging, and countdown timers.
 */

#ifndef INTEGRALL_TIME_MODULE_H
#define INTEGRALL_TIME_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

// Optional: include RTC support if user has the library installed
// Install via Library Manager: "RTClib" by Adafruit
#if __has_include(<RTClib.h>)
  #include <RTClib.h>
  #define INTEGRALL_RTC_AVAILABLE 1
#else
  #define INTEGRALL_RTC_AVAILABLE 0
#endif

namespace Integrall {

class TimeModule {
public:
    TimeModule() : _lastSyncMillis(0), _syncInterval(3600000) {}

    /**
     * Start the RTC module (usually DS3231)
     */
#if INTEGRALL_RTC_AVAILABLE
    bool beginRTC() {
        if (!_rtc.begin()) return false;
        if (_rtc.lostPower()) {
            _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
        return true;
    }

    /**
     * Get the current time as a formatted string (HH:MM:SS)
     */
    String getTimeString() {
        DateTime now = _rtc.now();
        char buf[9];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        return String(buf);
    }

    /**
     * Get full ISO formatted date-time
     */
    String getISODateTime() {
        DateTime now = _rtc.now();
        char buf[20];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d", 
          now.year(), now.month(), now.day(), 
          now.hour(), now.minute(), now.second());
        return String(buf);
    }
#endif

    /**
     * Setup NTP server sync (for ESP32/ESP8266)
     */
#if defined(ESP32) || defined(ESP8266)
    void setupNTP(const char* ntpServer = "pool.ntp.org", long gmtOffset_sec = 0, int daylightOffset_sec = 0) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }

    /**
     * Check if system clock is currently synced with network
     */
    bool isSyncDone() {
        struct tm timeinfo;
        return getLocalTime(&timeinfo);
    }
#endif

    /**
     * Read GPS Location (Latitude/Longitude)
     * Requires TinyGPS++ library and a Serial connection to the module.
     */
#if __has_include(<TinyGPS++.h>)
    void getGPSLocation(HardwareSerial& port, float &lat, float &lng) {
        #include <TinyGPS++.h>
        static TinyGPSPlus gps;
        while (port.available() > 0) {
            gps.encode(port.read());
        }
        if (gps.location.isValid()) {
            lat = gps.location.lat();
            lng = gps.location.lng();
        } else {
            lat = lng = 0.0f;
        }
    }
#endif

private:
#if INTEGRALL_RTC_AVAILABLE
    RTC_DS3231 _rtc;
#endif
    unsigned long _lastSyncMillis;
    uint32_t _syncInterval;
};

} // namespace Integrall

#endif // INTEGRALL_TIME_MODULE_H
