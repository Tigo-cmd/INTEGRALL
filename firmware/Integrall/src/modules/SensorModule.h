/**
 * SensorModule.h
 * 
 * Simple wrapper for common sensors (DHT11/22, HC-SR04, Analog)
 * Hides timing logic, pulse measuring, and boilerplate math.
 */

#ifndef INTEGRALL_SENSOR_MODULE_H
#define INTEGRALL_SENSOR_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_SENSORS_ENABLED || INTEGRALL_MODULE_IMU_ENABLED

#include <Arduino.h>

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
// so we must exclude it when the camera module is enabled
#if !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_Sensor.h>) && __has_include(<Adafruit_BME280.h>)
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define INTEGRALL_BME280_AVAILABLE 1
#else
#define INTEGRALL_BME280_AVAILABLE 0
#endif

namespace Integrall {

class SensorModule {
public:
    SensorModule() {}

    /**
     * Read generic analog sensor (0-100% scale)
     */
    int readAnalogPercent(uint8_t pin) {
        int val = analogRead(pin);
        #if defined(ESP32)
        return map(val, 0, 4095, 0, 100);
        #else
        return map(val, 0, 1023, 0, 100);
        #endif
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
     * Calculate altitude based on pressure
     */
    float calculateAltitude(float pressure, float seaLevelPressure = 1013.25) {
        return 44330.0 * (1.0 - pow(pressure / seaLevelPressure, 0.1903));
    }

    /**
     * Simple Dew Point approximation
     */
    float calculateDewPoint(float temp, float humidity) {
        return temp - ((100.0 - humidity) / 5.0);
    }

    /**
     * Simple Heat Index approximation
     */
    float calculateHeatIndex(float temp, float humidity) {
        return temp + 0.33 * humidity - 4.0;
    }

    /**
     * Get distance from HC-SR04 Ultrasonic Sensor (in CM)
     * Includes basic noise filtering (averaging).
     */
    float readDistanceCM(uint8_t trigPin, uint8_t echoPin, uint8_t samples = 3) {
        float total = 0;
        uint8_t validSamples = 0;

        for (uint8_t i = 0; i < samples; i++) {
            pinMode(trigPin, OUTPUT);
            pinMode(echoPin, INPUT);
            
            digitalWrite(trigPin, LOW);
            delayMicroseconds(2);
            digitalWrite(trigPin, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPin, LOW);
            
            long duration = pulseIn(echoPin, HIGH, 25000); // 25ms = ~4 meters
            if (duration > 0) {
                total += (duration * 0.034 / 2);
                validSamples++;
            }
            if (samples > 1) delay(10); // Short gap between bursts
        }
        
        return (validSamples > 0) ? (total / validSamples) : -1.0f;
    }

    /**
     * Check if an object is within a specific range (Smart Alert)
     */
    bool isWithinRange(uint8_t trig, uint8_t echo, float minCM, float maxCM) {
        float d = readDistanceCM(trig, echo, 1);
        return (d >= minCM && d <= maxCM);
    }

    /**
     * Helper to check if a simple digital sensor is triggered (e.g., PIR Motion, Flame, Tilt)
     */
    bool isTriggered(uint8_t pin, bool activeHigh = true) {
        pinMode(pin, INPUT);
        bool state = digitalRead(pin);
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
        OneWire oneWire(pin);
        DallasTemperature sensors(&oneWire);
        sensors.begin();
        sensors.requestTemperatures();
        float t = celsius ? sensors.getTempCByIndex(0) : sensors.getTempFByIndex(0);
        return (t == DEVICE_DISCONNECTED_C || t == DEVICE_DISCONNECTED_F) ? -999.0f : t;
    }
#else
    float readProbeTemp(uint8_t pin, bool celsius = true) { return -999.0f; }
#endif

    /**
     * Read BME280 Environment Sensor (Multi-function)
     * Requires Adafruit_BME280 library.
     */
#if INTEGRALL_BME280_AVAILABLE
    void readEnvironment(uint8_t addr, float &temp, float &hum, float &pres) {
        Adafruit_BME280 bme;
        if (!bme.begin(addr)) { temp = hum = pres = -1.0f; return; }
        temp = bme.readTemperature();
        hum = bme.readHumidity();
        pres = bme.readPressure() / 100.0f; // hPa
    }
#else
    void readEnvironment(uint8_t addr, float &temp, float &hum, float &pres) { temp = hum = pres = -1.0f; }
#endif

    /**
     * Read temperature from DHT11/DHT22 sensor
     * @param pin     Data pin
     * @param type    DHT11, DHT22, or DHT21
     * @param celsius true = Celsius, false = Fahrenheit
     */
#if INTEGRALL_DHT_AVAILABLE
    float readTemperature(uint8_t pin, uint8_t type = DHT22, bool celsius = true) {
        DHT dht(pin, type);
        dht.begin();
        delay(250); // DHT needs settle time
        float t = celsius ? dht.readTemperature() : dht.readTemperature(true);
        return isnan(t) ? -999.0f : t;
    }

    /**
     * Read humidity from DHT11/DHT22 sensor (returns 0-100%)
     */
    float readHumidity(uint8_t pin, uint8_t type = DHT22) {
        DHT dht(pin, type);
        dht.begin();
        delay(250);
        float h = dht.readHumidity();
        return isnan(h) ? -1.0f : h;
    }
#else
    float readTemperature(uint8_t pin, uint8_t type = 22, bool celsius = true) { return -999.0f; }
    float readHumidity(uint8_t pin, uint8_t type = 22) { return -1.0f; }
#endif

#if INTEGRALL_MODULE_IMU_ENABLED
    /**
     * Start the MPU6050 IMU
     * Includes the manual wake-up sequence from TestCode1.ino
     */
    bool beginIMU(uint8_t sda = 21, uint8_t scl = 22) {
        Wire.begin(sda, scl);
        
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
    }

    /**
     * Get total acceleration magnitude (Impact Force)
     */
    float getImpactForce() {
        if (!_imuStarted) return 0.0f;
        sensors_event_t a, g, temp;
        _mpu.getEvent(&a, &g, &temp);
        return sqrt((a.acceleration.x * a.acceleration.x) + 
                    (a.acceleration.y * a.acceleration.y) + 
                    (a.acceleration.z * a.acceleration.z));
    }

    /**
     * Determine if the sensor is in a horizontal position (fallen)
     * Calculates Pitch and Roll, returning true if tilt is > 60 degrees.
     */
    bool isHorizontal() {
        if (!_imuStarted) return false;
        sensors_event_t a, g, temp;
        _mpu.getEvent(&a, &g, &temp);
        
        float roll = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
        float pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
        
        return (abs(roll) > 60.0 || abs(pitch) > 60.0);
    }
    
    /**
     * Posture helper for telemetry ("Standing", "Sitting", "Fallen")
     * Simple mock logic: upright = standing, slight tilt = sitting, >60 = fallen
     */
    const char* getPostureString() {
        if (isHorizontal()) return "Fallen";
        // Calculate pitch to determine Sitting vs Standing
        sensors_event_t a, g, temp;
        _mpu.getEvent(&a, &g, &temp);
        float pitch = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
        // If tilted slightly forward (e.g. 20-45 degrees), might be sitting
        if (pitch > 20.0 && pitch < 60.0) return "Sitting";
        return "Standing";
    }
#endif

private:
#if INTEGRALL_MODULE_IMU_ENABLED
    Adafruit_MPU6050 _mpu;
    bool _imuStarted = false;
#endif
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_SENSORS_ENABLED || INTEGRALL_MODULE_IMU_ENABLED
#endif // INTEGRALL_SENSOR_MODULE_H
