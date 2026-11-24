/**
 * TimeModule.h
 *
 * Wall-clock time for the Integrall Framework:
 *  - NTP (ESP32/ESP8266, non-blocking: the SDK syncs in the background)
 *  - DS3231 RTC ("RTClib" library)
 *  - GPS location/time via TinyGPS++ ("TinyGPSPlus" library). The GPS UART is
 *    drained continuously in handle(), so fixes are always fresh.
 */

#ifndef INTEGRALL_TIME_MODULE_H
#define INTEGRALL_TIME_MODULE_H

#include <Arduino.h>
#include <time.h>
#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"
#include "../core/Util.h"

#if INTEGRALL_MODULE_TIME_ENABLED

#if __has_include(<RTClib.h>)
  #include <RTClib.h>
  #define INTEGRALL_RTC_AVAILABLE 1
#else
  #define INTEGRALL_RTC_AVAILABLE 0
#endif

#if __has_include(<TinyGPS++.h>)
  #include <TinyGPS++.h>
  #define INTEGRALL_GPS_AVAILABLE 1
#else
  #define INTEGRALL_GPS_AVAILABLE 0
#endif

namespace Integrall {

class TimeModule {
public:
    TimeModule() {}

    /** Called by System::handle(). Never blocks. */
    void handle() {
        #if INTEGRALL_GPS_AVAILABLE
        if (_gps_port) {
            int budget = 128;                         // bound the work per loop
            while (budget-- > 0 && _gps_port->available() > 0) _gps.encode(_gps_port->read());
        }
        #endif
        #if INTEGRALL_RTC_AVAILABLE && (defined(ESP32) || defined(ESP8266))
        if (_rtc_ok && _rtc_from_ntp && !_rtc_synced && isSyncDone()) {
            _rtc.adjust(DateTime((uint32_t)time(nullptr)));
            _rtc_synced = true;
            INTEGRALL_LOG_INFO("RTC set from NTP");
        }
        #endif
    }

    // ---------------------------------------------------------------- RTC ---
#if INTEGRALL_RTC_AVAILABLE
    /** Start a DS3231. @return false if it doesn't answer on I2C. */
    bool beginRTC() {
        Util::i2cBegin();
        if (!Util::i2cPresent(0x68) || !_rtc.begin(&Wire)) {
            INTEGRALL_LOG_WARN("DS3231 RTC not found on I2C 0x68");
            return false;
        }
        _rtc_ok = true;
        if (_rtc.lostPower()) {
            INTEGRALL_LOG_WARN("RTC lost power: time invalid until rtcAdjust()/NTP sync");
        }
        return true;
    }

    /** true if the RTC's backup battery failed since it was last set. */
    bool rtcLostPower() { return _rtc_ok && _rtc.lostPower(); }

    /** Set the RTC (e.g. rtcAdjust(DateTime(F(__DATE__), F(__TIME__))) ). */
    void rtcAdjust(const DateTime& dt) { if (_rtc_ok) _rtc.adjust(dt); }

    /** Automatically copy NTP time into the RTC once NTP has synced. */
    void rtcSyncFromNTP(bool enable = true) { _rtc_from_ntp = enable; _rtc_synced = false; }

    DateTime rtcNow() { return _rtc_ok ? _rtc.now() : DateTime((uint32_t)0); }
#endif

    /** "HH:MM:SS" from the RTC if present, else NTP clock, else "--:--:--". */
    String getTimeString() {
        int h, m, sec, y, mo, d;
        if (!_now(y, mo, d, h, m, sec)) return String("--:--:--");
        char buf[9];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, sec);
        return String(buf);
    }

    /** "YYYY-MM-DDTHH:MM:SS" (local time), or "" if no valid clock. */
    String getISODateTime() {
        int h, m, sec, y, mo, d;
        if (!_now(y, mo, d, h, m, sec)) return String("");
        char buf[20];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d", y, mo, d, h, m, sec);
        return String(buf);
    }

    // ---------------------------------------------------------------- NTP ---
#if defined(ESP32) || defined(ESP8266)
    /** Start background NTP sync (needs WiFi to be up eventually). Returns immediately. */
    void setupNTP(const char* ntpServer = "pool.ntp.org", long gmtOffset_sec = 0, int daylightOffset_sec = 0) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.google.com");
        _ntp_started = true;
    }

    /** true once the system clock holds a real date (after 2020-01-01). */
    bool isSyncDone() const { return time(nullptr) > 1577836800; }

    /** Seconds since the Unix epoch (UTC), 0 if not synced. */
    uint32_t epoch() const { time_t t = time(nullptr); return t > 1577836800 ? (uint32_t)t : 0; }
#endif

    // ---------------------------------------------------------------- GPS ---
#if INTEGRALL_GPS_AVAILABLE
    /**
     * Attach a GPS on a hardware UART. After this, handle() keeps the fix
     * up to date. ESP32 may pass rx/tx pins; other boards use the port's pins.
     */
    void beginGPS(HardwareSerial& port, uint32_t baud = 9600, int rx = -1, int tx = -1) {
        #if defined(ESP32)
        if (rx >= 0 && tx >= 0) port.begin(baud, SERIAL_8N1, rx, tx); else port.begin(baud);
        #else
        (void)rx; (void)tx;
        port.begin(baud);
        #endif
        _gps_port = &port;
    }

    bool gpsHasFix() { return _gps.location.isValid() && _gps.location.age() < 5000; }
    uint32_t gpsSatellites() { return _gps.satellites.isValid() ? _gps.satellites.value() : 0; }
    TinyGPSPlus& gps() { return _gps; }

    /**
     * Latest latitude/longitude. lat/lng are NAN without a valid fix.
     * (Legacy: if you never called beginGPS(), this drains `port` itself.)
     */
    void getGPSLocation(HardwareSerial& port, float& lat, float& lng) {
        if (!_gps_port) { int n = 128; while (n-- > 0 && port.available() > 0) _gps.encode(port.read()); }
        if (gpsHasFix()) { lat = (float)_gps.location.lat(); lng = (float)_gps.location.lng(); }
        else { lat = lng = NAN; }
    }
#endif

private:
#if INTEGRALL_RTC_AVAILABLE
    RTC_DS3231 _rtc;
    bool _rtc_ok = false;
    bool _rtc_from_ntp = false;
    bool _rtc_synced = false;
#endif
#if INTEGRALL_GPS_AVAILABLE
    TinyGPSPlus     _gps;
    HardwareSerial* _gps_port = nullptr;
#endif
    bool _ntp_started = false;

    bool _now(int& y, int& mo, int& d, int& h, int& m, int& s) {
        #if INTEGRALL_RTC_AVAILABLE
        if (_rtc_ok && !_rtc.lostPower()) {
            DateTime n = _rtc.now();
            y = n.year(); mo = n.month(); d = n.day(); h = n.hour(); m = n.minute(); s = n.second();
            return true;
        }
        #endif
        #if defined(ESP32) || defined(ESP8266)
        if (isSyncDone()) {
            time_t t = time(nullptr);
            struct tm tmv;
            localtime_r(&t, &tmv);
            y = tmv.tm_year + 1900; mo = tmv.tm_mon + 1; d = tmv.tm_mday;
            h = tmv.tm_hour; m = tmv.tm_min; s = tmv.tm_sec;
            return true;
        }
        #endif
        (void)y; (void)mo; (void)d; (void)h; (void)m; (void)s;
        return false;
    }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_TIME_ENABLED
#endif // INTEGRALL_TIME_MODULE_H
