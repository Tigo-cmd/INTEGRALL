/**
 * SensorModule.h
 * 
 * Simple wrapper for common sensors (DHT11/22, HC-SR04, DS18B20, BME280, MPU6050, Analog)
 * Hides timing logic, pulse measuring, instance caching, and boilerplate math.
 */

#ifndef INTEGRALL_SENSOR_MODULE_H
#define INTEGRALL_SENSOR_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_SENSORS_ENABLED || INTEGRALL_MODULE_IMU_ENABLED

#include <Arduino.h>
#include <math.h>

// DHT sensor support (no conflict with camera)
#if __has_include(<DHT.h>)
#include <DHT.h>
#define INTEGRALL_DHT_AVAILABLE 1
#else
#define INTEGRALL_DHT_AVAILABLE 0
#endif

// OneWire / DS18B20 support (no conflict with camera)
#if __has_include(<OneWire.h>) && __has_include(<DallasTemperature.h>)
#include <OneWire.h>
#include <DallasTemperature.h>
#define INTEGRALL_ONEWIRE_AVAILABLE 1
#else
#define INTEGRALL_ONEWIRE_AVAILABLE 0
#endif

// BME280 support — Adafruit_Sensor.h conflicts with esp_camera.h (both define sensor_t)
// so we exclude it when the camera module is enabled
#if !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_Sensor.h>) && __has_include(<Adafruit_BME280.h>)
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define INTEGRALL_BME280_AVAILABLE 1
#else
#define INTEGRALL_BME280_AVAILABLE 0
#endif

// MPU6050 IMU support — also uses Adafruit_Sensor.h
#if INTEGRALL_MODULE_IMU_ENABLED && !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_MPU6050.h>)
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#define INTEGRALL_MPU6050_AVAILABLE 1
#else
#define INTEGRALL_MPU6050_AVAILABLE 0
#endif

namespace Integrall {

class SensorModule {
public:
    SensorModule() {}

    ~SensorModule() {
        #if INTEGRALL_DHT_AVAILABLE
        if (_dht) { delete _dht; _dht = nullptr; }
        #endif
        #if INTEGRALL_ONEWIRE_AVAILABLE
        if (_dallas) { delete _dallas; _dallas = nullptr; }
        if (_oneWire) { delete _oneWire; _oneWire = nullptr; }
        #endif
        #if INTEGRALL_BME280_AVAILABLE
        if (_bme) { delete _bme; _bme = nullptr; }
        #endif
    }

    /**
     * Read generic analog sensor (0-100% scale)
     */
    int readAnalogPercent(uint8_t pin) {
        int val = analogRead(pin);
        #if defined(ESP32)
        int pct = map(val, 0, 4095, 0, 100);
        #else
        int pct = map(val, 0, 1023, 0, 100);
        #endif
        return constrain(pct, 0, 100);
    }

    /**
     * Read LDR Light intensity (0-100% scale)
     * @param pin           Analog pin
     * @param reverseScale  Set to true if 0 = Bright (common with pull-up LDRs)
     */
    int readLightPercent(uint8_t pin, bool reverseScale = false) {
        int p = readAnalogPercent(pin);
        return reverseScale ? (100 - p) : p;
    }

    /**
     * Calculate altitude (meters) based on atmospheric pressure (hPa)
     */
    float calculateAltitude(float pressure, float seaLevelPressure = 1013.25f) {
        if (pressure <= 0.0f || seaLevelPressure <= 0.0f) return 0.0f;
        return 44330.0f * (1.0f - powf(pressure / seaLevelPressure, 0.1903f));
    }

    /**
     * Calculate Dew Point (°C) using the Magnus-Tetens formula
     */
    float calculateDewPoint(float tempC, float humidity) {
        if (humidity <= 0.0f) return tempC;
        const float a = 17.27f;
        const float b = 237.7f;
        float alpha = ((a * tempC) / (b + tempC)) + logf(humidity / 100.0f);
        return (b * alpha) / (a - alpha);
    }

    /**
     * Calculate Heat Index (°C) using NOAA Steadman / Rothfusz regression
     */
    float calculateHeatIndex(float tempC, float humidity) {
        // Convert to Fahrenheit for NOAA formula
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

    /**
     * Get distance from HC-SR04 Ultrasonic Sensor (in CM)
     * Includes basic noise filtering (averaging).
     */
    float readDistanceCM(uint8_t trigPin, uint8_t echoPin, uint8_t samples = 3) {
        if (samples == 0) samples = 1;
        float total = 0;
        uint8_t validSamples = 0;

        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);

        for (uint8_t i = 0; i < samples; i++) {
            digitalWrite(trigPin, LOW);
            delayMicroseconds(2);
            digitalWrite(trigPin, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPin, LOW);
            
            unsigned long duration = pulseIn(echoPin, HIGH, 25000UL); // 25ms = ~4 meters
            if (duration > 0) {
                total += (duration * 0.0343f * 0.5f);
                validSamples++;
            }
            if (i + 1 < samples) delay(10); // Short gap between bursts
        }
        
        return (validSamples > 0) ? (total / validSamples) : -1.0f;
    }

    /**
     * Check if an object is within a specific range (Smart Alert)
     */
    bool isWithinRange(uint8_t trig, uint8_t echo, float minCM, float maxCM) {
        float d = readDistanceCM(trig, echo, 1);
        return (d >= 0.0f && d >= minCM && d <= maxCM);
    }

    /**
     * Helper to check if a simple digital sensor is triggered (e.g., PIR Motion, Flame, Tilt)
     */
    bool isTriggered(uint8_t pin, bool activeHigh = true) {
        pinMode(pin, INPUT);
        bool state = (digitalRead(pin) == HIGH);
        return activeHigh ? state : !state;
    }

    /**
     * Alias for isTriggered - clearer for Security/Motion logic
     */
    bool motionDetected(uint8_t pin) { return isTriggered(pin, true); }

    /**
     * Read Soil Moisture or MQ Gas sensors (normalized to 0-100%)
     */
    int readPercent(uint8_t pin, bool inverted = false) {
        int p = readAnalogPercent(pin);
        return inverted ? (100 - p) : p;
    }

    /**
     * Read DS18B20 Waterproof Temperature Sensor
     * Requires DallasTemperature and OneWire libraries.
     */
#if INTEGRALL_ONEWIRE_AVAILABLE
    float readProbeTemp(uint8_t pin, bool celsius = true) {
        if (!_dallas || _oneWirePin != pin) {
            if (_dallas) delete _dallas;
            if (_oneWire) delete _oneWire;
            _oneWire = new OneWire(pin);
            _dallas = new DallasTemperature(_oneWire);
            _dallas->begin();
            _oneWirePin = pin;
        }
        _dallas->requestTemperatures();
        float t = celsius ? _dallas->getTempCByIndex(0) : _dallas->getTempFByIndex(0);
        return (t == DEVICE_DISCONNECTED_C || t == DEVICE_DISCONNECTED_F) ? -999.0f : t;
    }
#else
    float readProbeTemp(uint8_t pin, bool celsius = true) {
        (void)pin; (void)celsius;
        return -999.0f;
    }
#endif

    /**
     * Read BME280 Environment Sensor (Multi-function)
     * Requires Adafruit_BME280 library.
     */
#if INTEGRALL_BME280_AVAILABLE
    void readEnvironment(uint8_t addr, float &temp, float &hum, float &pres) {
        if (!_bme || _bmeAddr != addr || !_bmeReady) {
            if (!_bme) _bme = new Adafruit_BME280();
            _bmeReady = _bme->begin(addr);
            _bmeAddr = addr;
            if (!_bmeReady) {
                temp = hum = pres = -1.0f;
                return;
            }
        }
        temp = _bme->readTemperature();
        hum = _bme->readHumidity();
        pres = _bme->readPressure() / 100.0f; // hPa
    }
#else
    void readEnvironment(uint8_t addr, float &temp, float &hum, float &pres) {
        (void)addr;
        temp = hum = pres = -1.0f;
    }
#endif

    /**
     * Read temperature from DHT11/DHT22 sensor
     * @param pin     Data pin
     * @param type    DHT11, DHT22, or DHT21
     * @param celsius true = Celsius, false = Fahrenheit
     */
#if INTEGRALL_DHT_AVAILABLE
    float readTemperature(uint8_t pin, uint8_t type = DHT22, bool celsius = true) {
        _ensureDHT(pin, type);
        float t = _dht->readTemperature(!celsius);
        return isnan(t) ? -999.0f : t;
    }

    /**
     * Read humidity from DHT11/DHT22 sensor (returns 0-100%)
     */
    float readHumidity(uint8_t pin, uint8_t type = DHT22) {
        _ensureDHT(pin, type);
        float h = _dht->readHumidity();
        return isnan(h) ? -1.0f : h;
    }
#else
    float readTemperature(uint8_t pin, uint8_t type = 22, bool celsius = true) {
        (void)pin; (void)type; (void)celsius;
        return -999.0f;
    }
    float readHumidity(uint8_t pin, uint8_t type = 22) {
        (void)pin; (void)type;
        return -1.0f;
    }
#endif

#if INTEGRALL_MODULE_IMU_ENABLED
    /**
     * Start the MPU6050 IMU
     */
    bool beginIMU(uint8_t sda = 21, uint8_t scl = 22) {
        #if INTEGRALL_MPU6050_AVAILABLE
        #if defined(ESP32) || defined(ESP8266)
        Wire.begin(sda, scl);
        #else
        (void)sda; (void)scl;
        Wire.begin();
        #endif
        
        // Wake MPU6050 manually
        Wire.beginTransmission(0x68);
        Wire.write(0x6B);
        Wire.write(0);
        Wire.endTransmission(true);
        delay(10); // Short settle time

        if (!_mpu.begin(0x68, &Wire, 0)) return false;
        
        _mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        _mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        _mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        _imuStarted = true;
        return true;
        #else
        (void)sda; (void)scl;
        return false;
        #endif
    }

    /**
     * Get total acceleration magnitude (Impact Force)
     */
    float getImpactForce() {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (!_imuStarted) return 0.0f;
        sensors_event_t a, g, temp;
        _mpu.getEvent(&a, &g, &temp);
        return sqrtf((a.acceleration.x * a.acceleration.x) + 
                     (a.acceleration.y * a.acceleration.y) + 
                     (a.acceleration.z * a.acceleration.z));
        #else
        return 0.0f;
        #endif
    }

    /**
     * Determine if the sensor is in a horizontal position (fallen)
     * Calculates Pitch and Roll, returning true if tilt is > 60 degrees.
     */
    bool isHorizontal() {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (!_imuStarted) return false;
        sensors_event_t a, g, temp;
        _mpu.getEvent(&a, &g, &temp);
        
        float roll = atan2f(a.acceleration.y, a.acceleration.z) * 180.0f / PI;
        float pitch = atan2f(-a.acceleration.x, sqrtf(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0f / PI;
        
        return (fabsf(roll) > 60.0f || fabsf(pitch) > 60.0f);
        #else
        return false;
        #endif
    }
    
    /**
     * Posture helper for telemetry ("Standing", "Sitting", "Fallen")
     */
    const char* getPostureString() {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (!_imuStarted) return "Unknown";
        if (isHorizontal()) return "Fallen";
        sensors_event_t a, g, temp;
        _mpu.getEvent(&a, &g, &temp);
        float pitch = atan2f(-a.acceleration.x, sqrtf(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0f / PI;
        if (pitch > 20.0f && pitch < 60.0f) return "Sitting";
        return "Standing";
        #else
        return "Unknown";
        #endif
    }
#endif

private:
#if INTEGRALL_DHT_AVAILABLE
    DHT* _dht = nullptr;
    uint8_t _dhtPin = 255;
    uint8_t _dhtType = 0;

    void _ensureDHT(uint8_t pin, uint8_t type) {
        if (!_dht || _dhtPin != pin || _dhtType != type) {
            if (_dht) delete _dht;
            _dht = new DHT(pin, type);
            _dht->begin();
            _dhtPin = pin;
            _dhtType = type;
            delay(50);
        }
    }
#endif

#if INTEGRALL_ONEWIRE_AVAILABLE
    OneWire* _oneWire = nullptr;
    DallasTemperature* _dallas = nullptr;
    uint8_t _oneWirePin = 255;
#endif

#if INTEGRALL_BME280_AVAILABLE
    Adafruit_BME280* _bme = nullptr;
    uint8_t _bmeAddr = 0;
    bool _bmeReady = false;
#endif

#if INTEGRALL_MPU6050_AVAILABLE
    Adafruit_MPU6050 _mpu;
    bool _imuStarted = false;
#endif
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_SENSORS_ENABLED || INTEGRALL_MODULE_IMU_ENABLED
#endif // INTEGRALL_SENSOR_MODULE_H
