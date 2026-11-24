/**
 * SensorModule.h
 *
 * Universal sensor drivers for the Integrall Framework.
 *
 * Contract (v1.2):
 *  - NOTHING here blocks loop() for long. Slow sensors are measured in the
 *    background (interrupts / handle() state machines) and the read*()
 *    functions return the most recent value immediately.
 *  - Float readers return NAN when the sensor is missing, still warming up,
 *    or the read failed. Test with Integrall::valid(x) or isnan(x).
 *    Int readers return -1 on failure.
 *  - Missing I2C sensors are re-probed with exponential back-off (1 s .. 30 s),
 *    not on every call.
 *
 * Interrupt-driven inputs (HC-SR04 echo, MH-Z19 PWM, flow meters) need a pin
 * that supports attachInterrupt(): any GPIO on ESP32/ESP8266, only D2/D3 on
 * Uno/Nano. HC-SR04 on other AVR pins falls back to a bounded pulseIn (<=25 ms).
 */

#ifndef INTEGRALL_SENSOR_MODULE_H
#define INTEGRALL_SENSOR_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_SENSORS_ENABLED || INTEGRALL_MODULE_IMU_ENABLED

#include <Arduino.h>
#include <math.h>
#include <new>
#include "../core/Logger.h"
#include "../core/Util.h"

#if __has_include(<DHT.h>)
  #include <DHT.h>
  #define INTEGRALL_DHT_AVAILABLE 1
#else
  #define INTEGRALL_DHT_AVAILABLE 0
#endif

#if __has_include(<OneWire.h>) && __has_include(<DallasTemperature.h>)
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #define INTEGRALL_ONEWIRE_AVAILABLE 1
#else
  #define INTEGRALL_ONEWIRE_AVAILABLE 0
#endif

// Adafruit_Sensor.h and esp_camera.h both define sensor_t, so Adafruit
// sensor drivers are unavailable in camera builds.
#if !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_Sensor.h>) && __has_include(<Adafruit_BME280.h>)
  #include <Adafruit_Sensor.h>
  #include <Adafruit_BME280.h>
  #define INTEGRALL_BME280_AVAILABLE 1
#else
  #define INTEGRALL_BME280_AVAILABLE 0
#endif

#if !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_Sensor.h>) && __has_include(<Adafruit_BMP280.h>)
  #include <Adafruit_Sensor.h>
  #include <Adafruit_BMP280.h>
  #define INTEGRALL_BMP280_AVAILABLE 1
#else
  #define INTEGRALL_BMP280_AVAILABLE 0
#endif

#if !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_VL53L0X.h>)
  #include <Adafruit_VL53L0X.h>
  #define INTEGRALL_VL53L0X_AVAILABLE 1
#else
  #define INTEGRALL_VL53L0X_AVAILABLE 0
#endif

#if INTEGRALL_MODULE_IMU_ENABLED && !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_MPU6050.h>)
  #include <Adafruit_Sensor.h>
  #include <Adafruit_MPU6050.h>
  #define INTEGRALL_MPU6050_AVAILABLE 1
#else
  #define INTEGRALL_MPU6050_AVAILABLE 0
#endif

#if defined(ESP32) || defined(ESP8266)
  #define INTEGRALL_ISR_ATTR IRAM_ATTR
#else
  #define INTEGRALL_ISR_ATTR
#endif

#ifndef INTEGRALL_MAX_DHT
  #if defined(__AVR__)
    #define INTEGRALL_MAX_DHT 2
  #else
    #define INTEGRALL_MAX_DHT 4
  #endif
#endif
#ifndef INTEGRALL_MAX_ONEWIRE_BUSES
  #if defined(__AVR__)
    #define INTEGRALL_MAX_ONEWIRE_BUSES 1
  #else
    #define INTEGRALL_MAX_ONEWIRE_BUSES 2
  #endif
#endif
#ifndef INTEGRALL_PULSE_SLOTS
  #if defined(__AVR__)
    #define INTEGRALL_PULSE_SLOTS 2    // Uno/Nano have only 2 external interrupt pins
  #else
    #define INTEGRALL_PULSE_SLOTS 4
  #endif
#endif

namespace Integrall {

// ============================================================================
// Interrupt-driven pulse capture (shared by ultrasonic, CO2 PWM, flow meters)
// ============================================================================
namespace Detail {

struct PulseSlot {
    volatile uint32_t rise_us  = 0;   // time of last rising edge
    volatile uint32_t high_us  = 0;   // last measured high time
    volatile uint32_t period_us = 0;  // last rising-to-rising period
    volatile uint32_t edges    = 0;   // rising-edge counter (flow meters)
    volatile bool     fresh    = false;
    uint8_t           pin      = 0xFF;
};

template <uint8_t N>
struct PulseIsr {
    static PulseSlot slot;
    static void INTEGRALL_ISR_ATTR onChange() {
        const uint32_t now = micros();
        if (digitalRead(slot.pin) == HIGH) {
            if (slot.rise_us) slot.period_us = now - slot.rise_us;
            slot.rise_us = now;
            slot.edges = slot.edges + 1;
        } else if (slot.rise_us) {
            slot.high_us = now - slot.rise_us;
            slot.fresh = true;
        }
    }
};
template <uint8_t N> PulseSlot PulseIsr<N>::slot;

static const uint8_t PULSE_SLOTS = INTEGRALL_PULSE_SLOTS;
static_assert(INTEGRALL_PULSE_SLOTS >= 1 && INTEGRALL_PULSE_SLOTS <= 4, "INTEGRALL_PULSE_SLOTS must be 1..4");

inline PulseSlot& pulseSlot(uint8_t i) {
    switch (i) {
        #if INTEGRALL_PULSE_SLOTS > 1
        case 1:  return PulseIsr<1>::slot;
        #endif
        #if INTEGRALL_PULSE_SLOTS > 2
        case 2:  return PulseIsr<2>::slot;
        #endif
        #if INTEGRALL_PULSE_SLOTS > 3
        case 3:  return PulseIsr<3>::slot;
        #endif
        default: return PulseIsr<0>::slot;
    }
}

typedef void (*IsrFn)();
inline IsrFn pulseIsr(uint8_t i) {
    switch (i) {
        #if INTEGRALL_PULSE_SLOTS > 1
        case 1:  return PulseIsr<1>::onChange;
        #endif
        #if INTEGRALL_PULSE_SLOTS > 2
        case 2:  return PulseIsr<2>::onChange;
        #endif
        #if INTEGRALL_PULSE_SLOTS > 3
        case 3:  return PulseIsr<3>::onChange;
        #endif
        default: return PulseIsr<0>::onChange;
    }
}

/** Find the slot already bound to pin, or bind a free one. -1 if impossible. */
inline int bindPulsePin(uint8_t pin, uint8_t mode = INPUT) {
    for (uint8_t i = 0; i < PULSE_SLOTS; i++) if (pulseSlot(i).pin == pin) return i;
    #if defined(NOT_AN_INTERRUPT)
    if (digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT) return -1;
    #endif
    for (uint8_t i = 0; i < PULSE_SLOTS; i++) {
        PulseSlot& s = pulseSlot(i);
        if (s.pin == 0xFF) {
            s.pin = pin;
            pinMode(pin, mode);
            attachInterrupt(digitalPinToInterrupt(pin), pulseIsr(i), CHANGE);
            return i;
        }
    }
    return -1;
}

/** Atomic snapshot of a slot's volatile fields. */
inline void pulseSnapshot(uint8_t i, uint32_t& high, uint32_t& period, uint32_t& edges, uint32_t& rise, bool& fresh) {
    PulseSlot& s = pulseSlot(i);
    noInterrupts();
    high = s.high_us; period = s.period_us; edges = s.edges; rise = s.rise_us; fresh = s.fresh;
    s.fresh = false;
    interrupts();
}

} // namespace Detail

// ============================================================================
// SensorModule
// ============================================================================
class SensorModule {
public:
    SensorModule() {}
    ~SensorModule() {
        #if INTEGRALL_DHT_AVAILABLE
        for (uint8_t i = 0; i < INTEGRALL_MAX_DHT; i++) if (_dht[i].dev) _dht[i].dev->~DHT();
        #endif
        #if INTEGRALL_ONEWIRE_AVAILABLE
        for (uint8_t i = 0; i < INTEGRALL_MAX_ONEWIRE_BUSES; i++) {
            if (_ow[i].dallas) _ow[i].dallas->~DallasTemperature();
            if (_ow[i].wire) _ow[i].wire->~OneWire();
        }
        #endif
    }

    // ------------------------------------------------------------------------
    // Background tick (called by System::handle). Never blocks.
    // ------------------------------------------------------------------------
    void handle() {
        const unsigned long now = millis();
        _handleUltrasonic(now);
        // Only set once readProbeTemp() is used, so the OneWire/Dallas driver is
        // linked only into sketches that actually read a DS18B20.
        if (_oneWireTick) _oneWireTick(this, now);
    }

    // ------------------------------------------------------------------------
    // ADC helpers
    // ------------------------------------------------------------------------
    static constexpr int getAdcMax() {
        #if defined(ESP32)
        return 4095;
        #else
        return 1023;
        #endif
    }

    static constexpr float getAdcRefVoltage() {
        #if defined(ESP32) || defined(ESP8266)
        return 3.3f;   // ESP8266: assumes the NodeMCU/Wemos 3.3 V divider on A0
        #else
        return 5.0f;
        #endif
    }

    /** Averaged pin voltage. */
    float readPinVoltage(uint8_t pin, uint8_t samples = 5, float vRef = getAdcRefVoltage()) {
        if (samples == 0) samples = 1;
        long sum = 0;
        for (uint8_t i = 0; i < samples; i++) sum += analogRead(pin);
        return ((float)sum / (float)samples) * vRef / (float)getAdcMax();
    }

    /** Analog input scaled to 0-100 %. */
    int readAnalogPercent(uint8_t pin) {
        long pct = map(analogRead(pin), 0, getAdcMax(), 0, 100);
        return (int)constrain(pct, 0L, 100L);
    }

    int readLightPercent(uint8_t pin, bool reverseScale = false) {
        int p = readAnalogPercent(pin);
        return reverseScale ? (100 - p) : p;
    }

    int readPercent(uint8_t pin, bool inverted = false) {
        int p = readAnalogPercent(pin);
        return inverted ? (100 - p) : p;
    }

    // ------------------------------------------------------------------------
    // Atmosphere math
    // ------------------------------------------------------------------------
    float calculateAltitude(float pressure, float seaLevelPressure = 1013.25f) {
        if (!(pressure > 0.0f) || !(seaLevelPressure > 0.0f)) return NAN;
        return 44330.0f * (1.0f - powf(pressure / seaLevelPressure, 0.1903f));
    }

    /** Dew point (°C), Magnus-Tetens. NAN for invalid input. */
    float calculateDewPoint(float tempC, float humidity) {
        if (isnan(tempC) || !(humidity > 0.0f) || humidity > 100.0f) return NAN;
        const float a = 17.27f, b = 237.7f;
        float alpha = ((a * tempC) / (b + tempC)) + logf(humidity / 100.0f);
        return (b * alpha) / (a - alpha);
    }

    /** Heat index (°C), NOAA Rothfusz regression with Steadman fallback. */
    float calculateHeatIndex(float tempC, float humidity) {
        if (isnan(tempC) || isnan(humidity)) return NAN;
        float T = tempC * 1.8f + 32.0f;
        float R = constrain(humidity, 0.0f, 100.0f);
        float hi = 0.5f * (T + 61.0f + ((T - 68.0f) * 1.2f) + (R * 0.094f));
        if (hi >= 80.0f) {
            hi = -42.379f + 2.04901523f * T + 10.14333127f * R
                 - 0.22475541f * T * R - 0.00683783f * T * T
                 - 0.05481717f * R * R + 0.00122874f * T * T * R
                 + 0.00085282f * T * R * R - 0.00000199f * T * T * R * R;
            if (R < 13.0f && T >= 80.0f && T <= 112.0f) {
                hi -= ((13.0f - R) / 4.0f) * sqrtf((17.0f - fabsf(T - 95.0f)) / 17.0f);
            } else if (R > 85.0f && T >= 80.0f && T <= 87.0f) {
                hi += ((R - 85.0f) / 10.0f) * ((87.0f - T) / 5.0f);
            }
        }
        return (hi - 32.0f) / 1.8f;
    }

    // ------------------------------------------------------------------------
    // HC-SR04 ultrasonic (asynchronous)
    // ------------------------------------------------------------------------
    /**
     * Distance in cm, or NAN if no echo yet / out of range (>4 m).
     * The first call registers the sensor; pings then run in the background
     * every 60 ms (datasheet minimum) and this returns the median of the last
     * `samples` (1-5) readings instantly.
     */
    float readDistanceCM(uint8_t trigPin, uint8_t echoPin, uint8_t samples = 3) {
        if (samples == 0) samples = 1;
        if (samples > US_HISTORY) samples = US_HISTORY;
        int u = _ultrasonicSlot(trigPin, echoPin);
        if (u < 0) return _pingBlocking(trigPin, echoPin);    // no interrupt on this echo pin
        return _ultrasonicMedian(_us[u], samples);
    }

    bool isWithinRange(uint8_t trig, uint8_t echo, float minCM, float maxCM) {
        float d = readDistanceCM(trig, echo, 1);
        return valid(d) && d >= minCM && d <= maxCM;
    }

    // ------------------------------------------------------------------------
    // Digital sensors (PIR, flame, tilt, reed, vibration, sound, hall, touch)
    // ------------------------------------------------------------------------
    bool isTriggered(uint8_t pin, bool activeHigh = true) {
        pinMode(pin, INPUT);
        bool high = (digitalRead(pin) == HIGH);
        return activeHigh ? high : !high;
    }
    bool motionDetected(uint8_t pin)                            { return isTriggered(pin, true); }
    bool flameDetected(uint8_t pin, bool activeLow = true)      { return isTriggered(pin, !activeLow); }
    bool tiltDetected(uint8_t pin, bool activeLow = true)       { return isTriggered(pin, !activeLow); }
    bool vibrationDetected(uint8_t pin, bool activeHigh = true) { return isTriggered(pin, activeHigh); }
    bool soundDetected(uint8_t pin, bool activeLow = true)      { return isTriggered(pin, !activeLow); }
    bool magneticDetected(uint8_t pin, bool activeLow = true)   { return isTriggered(pin, !activeLow); }
    bool touchDetected(uint8_t pin, bool activeHigh = true)     { return isTriggered(pin, activeHigh); }

    /** Reed switch to GND with pull-up: open door = HIGH. */
    bool doorOpen(uint8_t pin, bool usePullup = true) {
        pinMode(pin, usePullup ? INPUT_PULLUP : INPUT);
        return digitalRead(pin) == HIGH;
    }

    // ------------------------------------------------------------------------
    // Soil / rain
    // ------------------------------------------------------------------------
    /** Soil moisture 0-100 %. Pass raw ADC values for dry and wet to calibrate. */
    int readSoilMoisture(uint8_t pin, int dryRaw = -1, int wetRaw = -1) {
        if (dryRaw >= 0 && wetRaw >= 0 && dryRaw != wetRaw) {
            long pct = map(analogRead(pin), dryRaw, wetRaw, 0, 100);
            return (int)constrain(pct, 0L, 100L);
        }
        return readPercent(pin, true);
    }
    int  readRainPercent(uint8_t pin, bool activeLow = true) { return readPercent(pin, activeLow); }
    bool isRaining(uint8_t pin, int thresholdPct = 30, bool activeLow = true) { return readRainPercent(pin, activeLow) >= thresholdPct; }

    // ------------------------------------------------------------------------
    // Analog temperature
    // ------------------------------------------------------------------------
    /** NTC thermistor (°C) via Beta equation. NAN if open/short circuit. */
    float readThermistorC(uint8_t pin, float beta = 3950.0f, float r0 = 10000.0f, float seriesR = 10000.0f, bool ntcToGnd = true) {
        int raw = analogRead(pin);
        const int adcMax = getAdcMax();
        if (raw <= 0 || raw >= adcMax || beta <= 0.0f || r0 <= 0.0f || seriesR <= 0.0f) return NAN;
        float ratio = (float)raw / (float)(adcMax - raw);
        float rNtc = ntcToGnd ? (seriesR * ratio) : (seriesR / ratio);
        float invT = (1.0f / 298.15f) + (logf(rNtc / r0) / beta);
        if (invT <= 0.0f) return NAN;
        return (1.0f / invT) - 273.15f;
    }
    float readLM35C(uint8_t pin)  { return readPinVoltage(pin, 5) * 100.0f; }
    float readTMP36C(uint8_t pin) { return (readPinVoltage(pin, 5) - 0.5f) * 100.0f; }

    // ------------------------------------------------------------------------
    // BH1750 ambient light (I2C, no library)
    // ------------------------------------------------------------------------
    /**
     * Illuminance in lux, NAN if absent. Puts the sensor in continuous
     * high-resolution mode once; the first value is ready ~180 ms later.
     */
    float readLux(uint8_t addr = 0x23) {
        #if INTEGRALL_HAS_WIRE
        if (_bh_addr != addr) { _bh_addr = addr; _bh_ready_at = 0; _bh_backoff.reset(); }
        if (_bh_ready_at == 0) {
            if (!_bh_backoff.ready()) return NAN;
            Util::i2cBegin();
            Wire.beginTransmission(addr);
            Wire.write(0x01);                    // power on
            bool ok = Wire.endTransmission() == 0;
            if (ok) { Wire.beginTransmission(addr); Wire.write(0x10); ok = Wire.endTransmission() == 0; }  // continuous H-res
            if (!ok) { _bh_backoff.fail(); return NAN; }
            _bh_ready_at = millis() + 180;
            return NAN;
        }
        if ((long)(millis() - _bh_ready_at) < 0) return NAN;
        if (Wire.requestFrom(addr, (uint8_t)2) != 2) { _bh_ready_at = 0; _bh_backoff.fail(); return NAN; }
        uint16_t raw = ((uint16_t)Wire.read() << 8);
        raw |= Wire.read();
        return (float)raw / 1.2f;
        #else
        (void)addr; return NAN;
        #endif
    }

    // ------------------------------------------------------------------------
    // SHT30/31/35 (I2C, no library) — two-phase, non-blocking
    // ------------------------------------------------------------------------
    /**
     * Returns true and fills tempC/humidity when a fresh measurement is
     * available; otherwise returns the previous values (NAN before the
     * first one) and false. Call repeatedly; each measurement takes ~15 ms.
     */
    bool readSHT3x(uint8_t addr, float& tempC, float& humidity) {
        #if INTEGRALL_HAS_WIRE
        const unsigned long now = millis();
        if (_sht_addr != addr) { _sht_addr = addr; _sht_t = _sht_h = NAN; _sht_due = 0; _sht_backoff.reset(); }
        bool fresh = false;
        if (_sht_due != 0 && (long)(now - _sht_due) >= 0) {
            _sht_due = 0;
            if (Wire.requestFrom(addr, (uint8_t)6) == 6) {
                uint8_t b[6];
                for (uint8_t i = 0; i < 6; i++) b[i] = Wire.read();
                if (_crc8(b, 2) == b[2] && _crc8(b + 3, 2) == b[5]) {
                    uint16_t rawT = ((uint16_t)b[0] << 8) | b[1];
                    uint16_t rawH = ((uint16_t)b[3] << 8) | b[4];
                    _sht_t = -45.0f + 175.0f * ((float)rawT / 65535.0f);
                    _sht_h = constrain(100.0f * ((float)rawH / 65535.0f), 0.0f, 100.0f);
                    fresh = true;
                }
            } else {
                _sht_t = _sht_h = NAN;
                _sht_backoff.fail();
            }
        }
        if (_sht_due == 0 && _sht_backoff.ready()) {
            Util::i2cBegin();
            Wire.beginTransmission(addr);
            Wire.write(0x24); Wire.write(0x00);  // single shot, high repeatability, no clock stretch
            if (Wire.endTransmission() == 0) _sht_due = now + 16;
            else { _sht_t = _sht_h = NAN; _sht_backoff.fail(); }
        }
        tempC = _sht_t; humidity = _sht_h;
        return fresh;
        #else
        (void)addr; tempC = humidity = NAN; return false;
        #endif
    }

    // ------------------------------------------------------------------------
    // UV / color / IR / ToF
    // ------------------------------------------------------------------------
    /** UV index 0-15 from GUVA-S12SD (~0.1 V per index). */
    float readUVIndex(uint8_t pin) {
        return constrain(readPinVoltage(pin, 5) / 0.1f, 0.0f, 15.0f);
    }

    /** TCS3200/TCS230 colour, 0-255 per channel (bounded: <= 15 ms in the dark). */
    void readColorRGB(uint8_t s2Pin, uint8_t s3Pin, uint8_t outPin, uint8_t& r, uint8_t& g, uint8_t& b) {
        pinMode(s2Pin, OUTPUT); pinMode(s3Pin, OUTPUT); pinMode(outPin, INPUT);
        digitalWrite(s2Pin, LOW);  digitalWrite(s3Pin, LOW);  unsigned long pR = pulseIn(outPin, LOW, 5000UL);
        digitalWrite(s2Pin, HIGH); digitalWrite(s3Pin, HIGH); unsigned long pG = pulseIn(outPin, LOW, 5000UL);
        digitalWrite(s2Pin, LOW);  digitalWrite(s3Pin, HIGH); unsigned long pB = pulseIn(outPin, LOW, 5000UL);
        r = _colorScale(pR); g = _colorScale(pG); b = _colorScale(pB);
    }

    /** Sharp GP2Y0A21YK0F, 10-80 cm. NAN when out of range. */
    float readIRDistanceCM(uint8_t pin) {
        float v = readPinVoltage(pin, 5);
        if (v < 0.4f || v > 3.2f) return NAN;
        return constrain(29.988f * powf(v, -1.173f), 10.0f, 80.0f);
    }

    /** VL53L0X time-of-flight distance in mm (continuous mode), -1 if absent/out of range. */
    int readLaserDistanceMM() {
        #if INTEGRALL_VL53L0X_AVAILABLE
        if (!_tof) {
            if (!_tof_backoff.ready()) return -1;
            if (!Util::i2cPresent(0x29)) { _tof_backoff.fail(); return -1; }
            _tof = new (_tof_storage) Adafruit_VL53L0X();
            if (!_tof->begin(0x29, false, &Wire)) { _tof->~Adafruit_VL53L0X(); _tof = nullptr; _tof_backoff.fail(); return -1; }
            _tof->startRangeContinuous(50);
            _tof_backoff.reset();
        }
        if (_tof->isRangeComplete()) {
            uint16_t mm = _tof->readRangeResult();
            _tof_mm = (mm > 0 && mm < 2000) ? (int)mm : -1;   // 8190/8191 = out of range
        }
        return _tof_mm;
        #else
        return -1;
        #endif
    }

    // ------------------------------------------------------------------------
    // Gas / dust / CO2
    // ------------------------------------------------------------------------
    /** MQ-series gas PPM via power-law curve (default MQ-135 CO2-eq). NAN if out of range. */
    float readGasPPM(uint8_t pin, float r0 = 10.0f, float rl = 10.0f, float a = 116.602f, float b = -2.769f) {
        float v = readPinVoltage(pin, 5);
        const float vRef = getAdcRefVoltage();
        if (v <= 0.01f || v >= vRef || r0 <= 0.0f || rl <= 0.0f) return NAN;
        float ratio = (rl * ((vRef - v) / v)) / r0;
        return (ratio > 0.0f) ? a * powf(ratio, b) : NAN;
    }

    /** Sharp GP2Y1010AU0F dust density (µg/m³). Sampled at most every 10 ms. */
    float readDustDensity(uint8_t ledPin, uint8_t analogPin) {
        const unsigned long now = millis();
        if (_dust_last && now - _dust_last < 10) return _dust_value;
        _dust_last = now ? now : 1;
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW);           // LED on (active-low)
        delayMicroseconds(280);              // datasheet sampling point
        int raw = analogRead(analogPin);
        delayMicroseconds(40);
        digitalWrite(ledPin, HIGH);
        float v = ((float)raw * getAdcRefVoltage()) / (float)getAdcMax();
        float mg = 0.17f * v - 0.1f;
        _dust_value = (mg < 0.0f ? 0.0f : mg) * 1000.0f;
        return _dust_value;
    }

    /**
     * MH-Z19B CO2 (ppm) from its PWM output, measured by interrupt.
     * Returns -1 until the first full 1 s cycle is captured, or if the pin
     * can't take an interrupt.
     */
    int readCO2PWM(uint8_t pwmPin, int rangePPM = 5000) {
        int s = Detail::bindPulsePin(pwmPin);
        if (s < 0) { _warnNoInterrupt(pwmPin); return -1; }
        uint32_t high, period, edges, rise; bool fresh;
        Detail::pulseSnapshot(s, high, period, edges, rise, fresh);
        if (period < 900000UL || period > 1100000UL) return -1;   // 1004 ms nominal cycle
        if (micros() - rise > 2500000UL) return -1;               // sensor stopped
        const uint32_t tl = period - high;
        if (high + tl <= 4000UL) return -1;
        long ppm = (long)((uint64_t)rangePPM * (high > 2000UL ? high - 2000UL : 0) / (high + tl - 4000UL));
        return (int)constrain(ppm, 0L, (long)rangePPM);
    }

    // ------------------------------------------------------------------------
    // Water quality / flow
    // ------------------------------------------------------------------------
    float readTDS(uint8_t pin, float waterTempC = 25.0f) {
        float v = readPinVoltage(pin, 10);
        float k = 1.0f + 0.02f * (waterTempC - 25.0f);
        float cv = v / (k > 0.0f ? k : 1.0f);
        float tds = (133.42f * cv * cv * cv - 255.86f * cv * cv + 857.39f * cv) * 0.5f;
        return tds < 0.0f ? 0.0f : tds;
    }

    float readPH(uint8_t pin, float neutralVoltage = 2.50f, float slope = -5.70f) {
        return constrain(7.0f + (readPinVoltage(pin, 10) - neutralVoltage) * slope, 0.0f, 14.0f);
    }

    /** Turbidity NTU (sensor powered at 5 V; 3.3 V boards need a divider). */
    float readTurbidityNTU(uint8_t pin) {
        float v = readPinVoltage(pin, 10);
        if (v < 2.5f) return 3000.0f;
        if (v > 4.2f) return 0.0f;
        return constrain(-1120.4f * v * v + 5742.3f * v - 4352.9f, 0.0f, 3000.0f);
    }

    /**
     * Flow rate (L/min) from a hall-effect turbine (YF-S201: 7.5 pulses/s per L/min).
     * Pulses are counted by interrupt; the rate is updated about once a second.
     * Returns -1 if the pin can't take an interrupt, 0 before the first window.
     */
    float readWaterFlowLPM(uint8_t pin, float calibrationFactor = 7.5f) {
        if (calibrationFactor <= 0.0f) return NAN;
        int s = Detail::bindPulsePin(pin, INPUT_PULLUP);
        if (s < 0) { _warnNoInterrupt(pin); return -1.0f; }
        FlowWin& w = _flow[s];
        const unsigned long now = millis();
        uint32_t high, period, edges, rise; bool fresh;
        Detail::pulseSnapshot(s, high, period, edges, rise, fresh);
        if (w.t0 == 0) { w.t0 = now ? now : 1; w.e0 = edges; return 0.0f; }
        if (now - w.t0 >= 1000UL) {
            float hz = (float)(edges - w.e0) * 1000.0f / (float)(now - w.t0);
            w.lpm = hz / calibrationFactor;
            w.total_pulses += edges - w.e0;
            w.t0 = now; w.e0 = edges;
        }
        return w.lpm;
    }

    // ------------------------------------------------------------------------
    // HX711 load cell (no library) — never waits for data-ready
    // ------------------------------------------------------------------------
    /** Latest raw 24-bit reading. Returns the previous value if a new one isn't ready yet. */
    long readWeightRaw(uint8_t doutPin, uint8_t sckPin, uint32_t timeoutMs = 0) {
        (void)timeoutMs;   // kept for API compatibility; reads never wait now
        if (_hx_dout != doutPin || _hx_sck != sckPin) {
            _hx_dout = doutPin; _hx_sck = sckPin; _hx_have = false;
            pinMode(sckPin, OUTPUT); digitalWrite(sckPin, LOW);
            pinMode(doutPin, INPUT);
        }
        if (digitalRead(doutPin) == HIGH) return _hx_raw;     // conversion not ready
        uint32_t value = 0;
        noInterrupts();                                       // SCK high > 60 us would power the chip down
        for (uint8_t i = 0; i < 24; i++) {
            digitalWrite(sckPin, HIGH); delayMicroseconds(1);
            value = (value << 1) | (digitalRead(doutPin) ? 1UL : 0UL);
            digitalWrite(sckPin, LOW);  delayMicroseconds(1);
        }
        digitalWrite(sckPin, HIGH); delayMicroseconds(1);     // 25th pulse: channel A, gain 128
        digitalWrite(sckPin, LOW);
        interrupts();
        if (value & 0x800000UL) value |= 0xFF000000UL;
        _hx_raw = (long)(int32_t)value;
        _hx_have = true;
        return _hx_raw;
    }

    /** Weight in grams (NAN until the first sample arrives). */
    float readWeightGrams(uint8_t doutPin, uint8_t sckPin, long tareOffset = 0, float scaleFactor = 420.0f) {
        if (scaleFactor == 0.0f) return NAN;
        long raw = readWeightRaw(doutPin, sckPin);
        return _hx_have ? (float)(raw - tareOffset) / scaleFactor : NAN;
    }

    // ------------------------------------------------------------------------
    // Current / voltage / pulse
    // ------------------------------------------------------------------------
    /** ACS712 DC current (A). sensitivity: 185 (5 A), 100 (20 A), 66 (30 A) mV/A. */
    float readCurrentDC(uint8_t pin, float sensitivityMvPerA = 185.0f, float zeroPointV = -1.0f) {
        if (sensitivityMvPerA <= 0.0f) return NAN;
        if (zeroPointV < 0.0f) zeroPointV = getAdcRefVoltage() * 0.5f;
        return ((readPinVoltage(pin, 10) - zeroPointV) * 1000.0f) / sensitivityMvPerA;
    }

    /** ACS712 AC RMS current (A) from peak-to-peak over one mains cycle (default 20 ms). */
    float readCurrentAC(uint8_t pin, float sensitivityMvPerA = 185.0f, uint16_t windowMs = 20) {
        if (sensitivityMvPerA <= 0.0f) return NAN;
        if (windowMs > 20) windowMs = 20;    // policy: never sample longer than 20 ms
        int lo = getAdcMax(), hi = 0;
        const unsigned long start = millis();
        do {
            int v = analogRead(pin);
            if (v > hi) hi = v;
            if (v < lo) lo = v;
        } while (millis() - start < windowMs);
        float vpp = ((float)(hi - lo) * getAdcRefVoltage()) / (float)getAdcMax();
        return (vpp * 0.5f * 0.70710678f * 1000.0f) / sensitivityMvPerA;
    }

    float readVoltageDivider(uint8_t pin, float dividerRatio = 5.0f) { return readPinVoltage(pin, 5) * dividerRatio; }
    int   readHeartPulsePercent(uint8_t pin) { return readAnalogPercent(pin); }

    // ------------------------------------------------------------------------
    // DS18B20 (asynchronous conversions)
    // ------------------------------------------------------------------------
    /**
     * Temperature of the first DS18B20 on `pin`. Conversions run in the
     * background (~750 ms each); this returns the latest value instantly,
     * NAN until the first conversion finishes or if no probe answers.
     */
    float readProbeTemp(uint8_t pin, bool celsius = true) {
        #if INTEGRALL_ONEWIRE_AVAILABLE
        _oneWireTick = [](SensorModule* self, unsigned long now) { self->_handleOneWire(now); };
        int b = _oneWireBus(pin);
        if (b < 0) return NAN;
        float c = _ow[b].lastC;
        if (isnan(c)) return NAN;
        return celsius ? c : (c * 1.8f + 32.0f);
        #else
        (void)pin; (void)celsius;
        return NAN;
        #endif
    }

    // ------------------------------------------------------------------------
    // BME280 / BMP280
    // ------------------------------------------------------------------------
    /** BME280 temperature (°C), humidity (%), pressure (hPa). NAN if absent. */
    void readEnvironment(uint8_t addr, float& temp, float& hum, float& pres) {
        temp = hum = pres = NAN;
        #if INTEGRALL_BME280_AVAILABLE
        if (!_bmeReady || _bmeAddr != addr) {
            if (_bmeAddr != addr) { _bmeAddr = addr; _bme_backoff.reset(); }
            if (!_bme_backoff.ready()) return;
            if (!Util::i2cPresent(addr) || !_bme.begin(addr, &Wire)) { _bmeReady = false; _bme_backoff.fail(); return; }
            _bmeReady = true;
            _bme_backoff.reset();
        }
        temp = _bme.readTemperature();
        hum  = _bme.readHumidity();
        pres = _bme.readPressure() / 100.0f;
        if (isnan(temp)) { _bmeReady = false; _bme_backoff.fail(); }
        #else
        (void)addr;
        #endif
    }

    /** Pressure in hPa from a BME280 or BMP280 at addr. NAN if neither answers. */
    float readPressure(uint8_t addr = 0x76) {
        #if INTEGRALL_BME280_AVAILABLE
        {
            float t, h, p;
            readEnvironment(addr, t, h, p);
            if (valid(p)) return p;
        }
        #endif
        #if INTEGRALL_BMP280_AVAILABLE
        if (!_bmpReady || _bmpAddr != addr) {
            if (_bmpAddr != addr) { _bmpAddr = addr; _bmp_backoff.reset(); }
            if (!_bmp_backoff.ready()) return NAN;
            if (!Util::i2cPresent(addr) || !_bmp.begin(addr)) { _bmpReady = false; _bmp_backoff.fail(); return NAN; }
            _bmpReady = true;
            _bmp_backoff.reset();
        }
        return _bmp.readPressure() / 100.0f;
        #else
        (void)addr;
        return NAN;
        #endif
    }

    // ------------------------------------------------------------------------
    // DHT11 / DHT22 (several sensors supported)
    // ------------------------------------------------------------------------
    /**
     * DHT temperature. The DHT library enforces the sensor's minimum sampling
     * interval (1 s DHT11 / 2 s DHT22) and returns its cached value between
     * reads. NAN if the sensor doesn't answer.
     */
    float readTemperature(uint8_t pin, uint8_t type = 22, bool celsius = true) {
        #if INTEGRALL_DHT_AVAILABLE
        DHT* d = _dhtFor(pin, type);
        return d ? d->readTemperature(!celsius) : NAN;
        #else
        (void)pin; (void)type; (void)celsius;
        return NAN;
        #endif
    }

    /** DHT relative humidity (0-100 %), NAN on failure. */
    float readHumidity(uint8_t pin, uint8_t type = 22) {
        #if INTEGRALL_DHT_AVAILABLE
        DHT* d = _dhtFor(pin, type);
        return d ? d->readHumidity() : NAN;
        #else
        (void)pin; (void)type;
        return NAN;
        #endif
    }

#if INTEGRALL_MODULE_IMU_ENABLED
    // ------------------------------------------------------------------------
    // MPU6050 IMU
    // ------------------------------------------------------------------------
    /**
     * Start the MPU6050 on the shared I2C bus. On ESP boards you may pass
     * SDA/SCL pins (only effective if the bus hasn't been started yet).
     */
    bool beginIMU(int sda = -1, int scl = -1, uint8_t addr = 0x68) {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (sda >= 0 && scl >= 0 && !Util::i2cState().started) Util::i2cSetPins(sda, scl);
        Util::i2cBegin();
        if (!Util::i2cPresent(addr)) {
            INTEGRALL_LOG_WARN("MPU6050 not found on I2C (0x68/0x69)");
            return false;
        }
        if (!_mpu.begin(addr, &Wire, 0)) return false;
        _mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        _mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        _mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        _imuStarted = true;
        _imuSampleAt = 0;
        return true;
        #else
        (void)sda; (void)scl; (void)addr;
        return false;
        #endif
    }

    bool imuReady() const {
        #if INTEGRALL_MPU6050_AVAILABLE
        return _imuStarted;
        #else
        return false;
        #endif
    }

    /** Magnitude of acceleration in m/s² (≈9.8 at rest). NAN if not started. */
    float getImpactForce() {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (!_imuSample()) return NAN;
        const auto& a = _imuA.acceleration;
        return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
        #else
        return NAN;
        #endif
    }

    float getPitch() {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (!_imuSample()) return NAN;
        const auto& a = _imuA.acceleration;
        return atan2f(-a.x, sqrtf(a.y * a.y + a.z * a.z)) * 180.0f / PI;
        #else
        return NAN;
        #endif
    }

    float getRoll() {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (!_imuSample()) return NAN;
        const auto& a = _imuA.acceleration;
        return atan2f(a.y, a.z) * 180.0f / PI;
        #else
        return NAN;
        #endif
    }

    /** true if tilted more than 60° in pitch or roll. */
    bool isHorizontal() {
        float p = getPitch(), r = getRoll();
        return valid(p) && valid(r) && (fabsf(r) > 60.0f || fabsf(p) > 60.0f);
    }

    /** "Standing", "Sitting", "Fallen" or "Unknown". */
    const char* getPostureString() {
        float p = getPitch();
        if (!valid(p)) return "Unknown";
        if (isHorizontal()) return "Fallen";
        if (p > 20.0f && p < 60.0f) return "Sitting";
        return "Standing";
    }

    float getIMUTemperature() {
        #if INTEGRALL_MPU6050_AVAILABLE
        return _imuSample() ? _imuT.temperature : NAN;
        #else
        return NAN;
        #endif
    }
#endif

private:
    void (*_oneWireTick)(SensorModule*, unsigned long) = nullptr;

    // ---- ultrasonic ----------------------------------------------------------
    static const uint8_t US_SENSORS = 2;
    static const uint8_t US_HISTORY = 5;
    struct Ultrasonic {
        uint8_t       trig = 0xFF, echo = 0xFF;
        int8_t        slot = -1;
        unsigned long last_ping = 0;
        bool          awaiting = false;      // pinged, echo not yet captured
        float         hist[US_HISTORY] = {NAN, NAN, NAN, NAN, NAN};
        uint8_t       head = 0;
    };
    Ultrasonic _us[US_SENSORS];

    int _ultrasonicSlot(uint8_t trig, uint8_t echo) {
        for (uint8_t i = 0; i < US_SENSORS; i++) if (_us[i].trig == trig && _us[i].echo == echo) return _us[i].slot >= 0 ? i : -1;
        for (uint8_t i = 0; i < US_SENSORS; i++) {
            if (_us[i].trig == 0xFF) {
                _us[i].trig = trig; _us[i].echo = echo;
                pinMode(trig, OUTPUT); digitalWrite(trig, LOW);
                _us[i].slot = (int8_t)Detail::bindPulsePin(echo);
                if (_us[i].slot < 0) { INTEGRALL_LOG_INFO_VAL("HC-SR04: echo pin has no interrupt, using pulseIn on pin ", echo); }
                return _us[i].slot >= 0 ? i : -1;
            }
        }
        return -1;
    }

    void _handleUltrasonic(unsigned long now) {
        for (uint8_t i = 0; i < US_SENSORS; i++) {
            Ultrasonic& u = _us[i];
            if (u.slot < 0) continue;
            uint32_t high, period, edges, rise; bool fresh;
            Detail::pulseSnapshot(u.slot, high, period, edges, rise, fresh);
            if (fresh && u.awaiting) {
                u.awaiting = false;
                _usPush(u, (high > 0 && high < 25000UL) ? high * 0.0343f * 0.5f : NAN);
            }
            if (u.awaiting && now - u.last_ping >= 200) {   // no echo at all: out of range / unplugged
                u.awaiting = false;
                _usPush(u, NAN);
            }
            if (!u.awaiting && (u.last_ping == 0 || now - u.last_ping >= 60)) {   // datasheet: >= 60 ms cycle
                u.last_ping = now ? now : 1;
                u.awaiting = true;
                digitalWrite(u.trig, LOW);  delayMicroseconds(2);
                digitalWrite(u.trig, HIGH); delayMicroseconds(10);
                digitalWrite(u.trig, LOW);
            }
        }
    }

    static void _usPush(Ultrasonic& u, float cm) {
        u.hist[u.head] = cm;
        u.head = (uint8_t)((u.head + 1) % US_HISTORY);
    }

    float _ultrasonicMedian(const Ultrasonic& u, uint8_t n) {
        float v[US_HISTORY]; uint8_t k = 0;
        for (uint8_t i = 0; i < n; i++) {
            float x = u.hist[(u.head + US_HISTORY - 1 - i) % US_HISTORY];
            if (!isnan(x)) v[k++] = x;
        }
        if (k == 0) return NAN;
        for (uint8_t i = 1; i < k; i++) { float t = v[i]; int8_t j = i - 1; while (j >= 0 && v[j] > t) { v[j + 1] = v[j]; j--; } v[j + 1] = t; }
        return v[k / 2];
    }

    float _pingBlocking(uint8_t trig, uint8_t echo) {
        pinMode(trig, OUTPUT); pinMode(echo, INPUT);
        digitalWrite(trig, LOW);  delayMicroseconds(2);
        digitalWrite(trig, HIGH); delayMicroseconds(10);
        digitalWrite(trig, LOW);
        unsigned long d = pulseIn(echo, HIGH, 25000UL);
        return d ? d * 0.0343f * 0.5f : NAN;
    }

    // ---- flow ----------------------------------------------------------------
    struct FlowWin { unsigned long t0 = 0; uint32_t e0 = 0; float lpm = 0.0f; uint32_t total_pulses = 0; };
    FlowWin _flow[Detail::PULSE_SLOTS];

    // ---- misc caches ---------------------------------------------------------
    unsigned long _dust_last = 0;
    float         _dust_value = NAN;

    uint8_t       _bh_addr = 0;
    unsigned long _bh_ready_at = 0;
    Util::Backoff _bh_backoff;

    uint8_t       _sht_addr = 0;
    unsigned long _sht_due = 0;
    float         _sht_t = NAN, _sht_h = NAN;
    Util::Backoff _sht_backoff;

    uint8_t _hx_dout = 0xFF, _hx_sck = 0xFF;
    long    _hx_raw = 0;
    bool    _hx_have = false;

    bool _warned_irq = false;
    void _warnNoInterrupt(uint8_t pin) {
        if (_warned_irq) return;
        _warned_irq = true;
        INTEGRALL_LOG_ERROR_VAL("Pin can't take an interrupt (Uno/Nano: use D2 or D3): ", pin);
    }

    static uint8_t _colorScale(unsigned long p) {
        if (p == 0) return 0;
        long v = map((long)p, 200, 20, 0, 255);
        return (uint8_t)constrain(v, 0L, 255L);
    }

    static uint8_t _crc8(const uint8_t* d, uint8_t n) {
        uint8_t crc = 0xFF;
        for (uint8_t i = 0; i < n; i++) {
            crc ^= d[i];
            for (uint8_t b = 0; b < 8; b++) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
        return crc;
    }

#if INTEGRALL_DHT_AVAILABLE
    struct DhtSlot {
        uint8_t pin = 0xFF, type = 0;
        DHT*    dev = nullptr;
        alignas(DHT) uint8_t storage[sizeof(DHT)];
    };
    DhtSlot _dht[INTEGRALL_MAX_DHT];

    DHT* _dhtFor(uint8_t pin, uint8_t type) {
        for (uint8_t i = 0; i < INTEGRALL_MAX_DHT; i++) {
            if (_dht[i].dev && _dht[i].pin == pin) {
                if (_dht[i].type == type) return _dht[i].dev;
                _dht[i].dev->~DHT(); _dht[i].dev = nullptr;   // type changed on same pin
                return _dhtCreate(_dht[i], pin, type);
            }
        }
        for (uint8_t i = 0; i < INTEGRALL_MAX_DHT; i++) if (!_dht[i].dev) return _dhtCreate(_dht[i], pin, type);
        INTEGRALL_LOG_ERROR("Too many DHT sensors (raise INTEGRALL_MAX_DHT)");
        return nullptr;
    }

    DHT* _dhtCreate(DhtSlot& s, uint8_t pin, uint8_t type) {
        s.dev = new (s.storage) DHT(pin, type);
        s.dev->begin();
        s.pin = pin; s.type = type;
        return s.dev;
    }
#endif

#if INTEGRALL_ONEWIRE_AVAILABLE
    struct OneWireBus {
        uint8_t            pin = 0xFF;
        OneWire*           wire = nullptr;
        DallasTemperature* dallas = nullptr;
        alignas(OneWire) uint8_t wire_storage[sizeof(OneWire)];
        alignas(DallasTemperature) uint8_t dallas_storage[sizeof(DallasTemperature)];
        unsigned long      requested_at = 0;
        bool               converting = false;
        float              lastC = NAN;
        Util::Backoff      backoff;
    };
    OneWireBus _ow[INTEGRALL_MAX_ONEWIRE_BUSES];

    int _oneWireBus(uint8_t pin) {
        for (uint8_t i = 0; i < INTEGRALL_MAX_ONEWIRE_BUSES; i++) if (_ow[i].pin == pin) return i;
        for (uint8_t i = 0; i < INTEGRALL_MAX_ONEWIRE_BUSES; i++) {
            OneWireBus& b = _ow[i];
            if (b.pin != 0xFF) continue;
            b.pin = pin;
            b.wire = new (b.wire_storage) OneWire(pin);
            b.dallas = new (b.dallas_storage) DallasTemperature(b.wire);
            b.dallas->begin();
            b.dallas->setResolution(12);
            b.dallas->setWaitForConversion(false);   // async: we poll for completion
            _oneWireStart(b);
            return i;
        }
        INTEGRALL_LOG_ERROR("Too many OneWire buses (raise INTEGRALL_MAX_ONEWIRE_BUSES)");
        return -1;
    }

    void _oneWireStart(OneWireBus& b) {
        if (b.dallas->getDeviceCount() == 0) {
            b.dallas->begin();                       // re-enumerate (probe may have been plugged in)
            if (b.dallas->getDeviceCount() == 0) { b.lastC = NAN; b.converting = false; b.backoff.fail(); return; }
            b.dallas->setResolution(12);
            b.dallas->setWaitForConversion(false);
        }
        b.backoff.reset();
        b.dallas->requestTemperatures();
        b.requested_at = millis();
        b.converting = true;
    }

    void _handleOneWire(unsigned long now) {
        for (uint8_t i = 0; i < INTEGRALL_MAX_ONEWIRE_BUSES; i++) {
            OneWireBus& b = _ow[i];
            if (b.pin == 0xFF) continue;
            if (!b.converting) {
                if (b.backoff.ready()) _oneWireStart(b);
                continue;
            }
            if (now - b.requested_at < 750) continue;         // 12-bit conversion time
            float c = b.dallas->getTempCByIndex(0);
            b.lastC = (c == DEVICE_DISCONNECTED_C || c < -55.0f || c > 125.0f) ? NAN : c;
            b.converting = false;
            if (isnan(b.lastC)) b.backoff.fail(); else _oneWireStart(b);
        }
    }
#endif

#if INTEGRALL_BME280_AVAILABLE
    Adafruit_BME280 _bme;
    uint8_t         _bmeAddr = 0;
    bool            _bmeReady = false;
    Util::Backoff   _bme_backoff;
#endif

#if INTEGRALL_BMP280_AVAILABLE
    Adafruit_BMP280 _bmp;
    uint8_t         _bmpAddr = 0;
    bool            _bmpReady = false;
    Util::Backoff   _bmp_backoff;
#endif

#if INTEGRALL_VL53L0X_AVAILABLE
    Adafruit_VL53L0X* _tof = nullptr;
    alignas(Adafruit_VL53L0X) uint8_t _tof_storage[sizeof(Adafruit_VL53L0X)];
    int               _tof_mm = -1;
    Util::Backoff     _tof_backoff;
#endif

#if INTEGRALL_MPU6050_AVAILABLE
    Adafruit_MPU6050 _mpu;
    bool             _imuStarted = false;
    unsigned long    _imuSampleAt = 0;
    sensors_event_t  _imuA, _imuG, _imuT;

    /** Read all IMU channels at most every 10 ms; getters share the sample. */
    bool _imuSample() {
        if (!_imuStarted) return false;
        const unsigned long now = millis();
        if (_imuSampleAt == 0 || now - _imuSampleAt >= 10) {
            _mpu.getEvent(&_imuA, &_imuG, &_imuT);
            _imuSampleAt = now ? now : 1;
        }
        return true;
    }
#endif
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_SENSORS_ENABLED || INTEGRALL_MODULE_IMU_ENABLED
#endif // INTEGRALL_SENSOR_MODULE_H
