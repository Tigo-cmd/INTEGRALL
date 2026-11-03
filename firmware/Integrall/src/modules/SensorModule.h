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

#if __has_include(<Wire.h>)
#include <Wire.h>
#define INTEGRALL_WIRE_AVAILABLE 1
#else
#define INTEGRALL_WIRE_AVAILABLE 0
#endif

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
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define INTEGRALL_BME280_AVAILABLE 1
#else
#define INTEGRALL_BME280_AVAILABLE 0
#endif

// BMP280 support
#if !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_Sensor.h>) && __has_include(<Adafruit_BMP280.h>)
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#define INTEGRALL_BMP280_AVAILABLE 1
#else
#define INTEGRALL_BMP280_AVAILABLE 0
#endif

// VL53L0X Laser Time-of-Flight (ToF) distance sensor support
#if !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_VL53L0X.h>)
#include <Adafruit_VL53L0X.h>
#define INTEGRALL_VL53L0X_AVAILABLE 1
#else
#define INTEGRALL_VL53L0X_AVAILABLE 0
#endif

// MPU6050 IMU support — also uses Adafruit_Sensor.h
#if INTEGRALL_MODULE_IMU_ENABLED && !INTEGRALL_MODULE_CAMERA_ENABLED && __has_include(<Adafruit_MPU6050.h>)
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
        #if INTEGRALL_BMP280_AVAILABLE
        if (_bmp) { delete _bmp; _bmp = nullptr; }
        #endif
        #if INTEGRALL_VL53L0X_AVAILABLE
        if (_tof) { delete _tof; _tof = nullptr; }
        #endif
    }

    /**
     * Platform-aware ADC resolution and reference voltage helpers
     */
    static constexpr int getAdcMax() {
        #if defined(ESP32)
        return 4095;
        #else
        return 1023;
        #endif
    }

    static constexpr float getAdcRefVoltage() {
        #if defined(ESP32) || defined(ESP8266)
        return 3.3f;
        #else
        return 5.0f;
        #endif
    }

    /**
     * Read averaged analog voltage on a pin
     */
    float readPinVoltage(uint8_t pin, uint8_t samples = 5, float vRef = getAdcRefVoltage()) {
        if (samples == 0) samples = 1;
        long sum = 0;
        for (uint8_t i = 0; i < samples; i++) {
            sum += analogRead(pin);
        }
        float avg = (float)sum / (float)samples;
        return (avg * vRef) / (float)getAdcMax();
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
     * Semantic digital sensor helpers (Flame, Tilt, Reed Door Switch, Vibration, Sound, Hall Effect)
     */
    bool flameDetected(uint8_t pin, bool activeLow = true)     { return isTriggered(pin, !activeLow); }
    bool tiltDetected(uint8_t pin, bool activeLow = true)      { return isTriggered(pin, !activeLow); }
    bool vibrationDetected(uint8_t pin, bool activeHigh = true){ return isTriggered(pin, activeHigh); }
    bool soundDetected(uint8_t pin, bool activeLow = true)     { return isTriggered(pin, !activeLow); }
    bool magneticDetected(uint8_t pin, bool activeLow = true)  { return isTriggered(pin, !activeLow); }
    bool touchDetected(uint8_t pin, bool activeHigh = true)    { return isTriggered(pin, activeHigh); }

    /**
     * Check if a magnetic reed door/window switch is open (uses INPUT_PULLUP by default)
     */
    bool doorOpen(uint8_t pin, bool usePullup = true) {
        pinMode(pin, usePullup ? INPUT_PULLUP : INPUT);
        return (digitalRead(pin) == HIGH);
    }

    /**
     * Read Soil Moisture or MQ Gas sensors (normalized to 0-100%)
     */
    int readPercent(uint8_t pin, bool inverted = false) {
        int p = readAnalogPercent(pin);
        return inverted ? (100 - p) : p;
    }

    /**
     * Read Capacitive or Resistive Soil Moisture (0-100%)
     * Supports optional dry/wet raw ADC calibration points; defaults to inverted analog percent.
     */
    int readSoilMoisture(uint8_t pin, int dryRaw = -1, int wetRaw = -1) {
        if (dryRaw >= 0 && wetRaw >= 0 && dryRaw != wetRaw) {
            int raw = analogRead(pin);
            int pct = map(raw, dryRaw, wetRaw, 0, 100);
            return constrain(pct, 0, 100);
        }
        return readPercent(pin, true);
    }

    /**
     * Read Rain / Water Level sensor (0-100%) and rain threshold check
     */
    int readRainPercent(uint8_t pin, bool activeLow = true) {
        return readPercent(pin, activeLow);
    }

    bool isRaining(uint8_t pin, int thresholdPct = 30, bool activeLow = true) {
        return readRainPercent(pin, activeLow) >= thresholdPct;
    }

    /**
     * Read NTC Thermistor Temperature (°C) using Beta Steinhart-Hart equation
     * @param pin      Analog pin connected to voltage divider
     * @param beta     Beta coefficient (usually 3950)
     * @param r0       Nominal resistance at 25°C (usually 10000 or 100000 ohms)
     * @param seriesR  Series resistor value in ohms (usually 10000)
     * @param ntcToGnd true if NTC is wired between pin and GND; false if between VCC and pin
     */
    float readThermistorC(uint8_t pin, float beta = 3950.0f, float r0 = 10000.0f, float seriesR = 10000.0f, bool ntcToGnd = true) {
        int raw = analogRead(pin);
        int adcMax = getAdcMax();
        if (raw <= 0 || raw >= adcMax || beta <= 0.0f || r0 <= 0.0f || seriesR <= 0.0f) return -999.0f;
        float ratio = (float)raw / (float)(adcMax - raw);
        float rNtc = ntcToGnd ? (seriesR * ratio) : (seriesR / ratio);
        float invT = (1.0f / 298.15f) + (logf(rNtc / r0) / beta);
        if (invT <= 0.0f) return -999.0f;
        return (1.0f / invT) - 273.15f;
    }

    /**
     * Read LM35 Analog Temperature Sensor (°C, 10mV/°C)
     */
    float readLM35C(uint8_t pin) {
        float v = readPinVoltage(pin, 5);
        return v * 100.0f;
    }

    /**
     * Read TMP36 Analog Temperature Sensor (°C, 500mV offset, 10mV/°C)
     */
    float readTMP36C(uint8_t pin) {
        float v = readPinVoltage(pin, 5);
        return (v - 0.5f) * 100.0f;
    }

    /**
     * Read BH1750 Digital Ambient Light Sensor in Lux (I2C, 0x23 or 0x5C)
     * Uses native I2C protocol — zero external libraries required!
     */
    float readLux(uint8_t addr = 0x23) {
        #if INTEGRALL_WIRE_AVAILABLE
        Wire.begin();
        Wire.beginTransmission(addr);
        Wire.write(0x10); // Continuous High-Res Mode (1 lx resolution)
        if (Wire.endTransmission() != 0) return -1.0f;
        if (Wire.requestFrom(addr, (uint8_t)2) != 2) return -1.0f;
        uint16_t raw = ((uint16_t)Wire.read() << 8) | Wire.read();
        return (float)raw / 1.2f;
        #else
        (void)addr;
        return -1.0f;
        #endif
    }

    /**
     * Read SHT30 / SHT31 / SHT35 High-Precision I2C Temperature & Humidity Sensor (0x44 or 0x45)
     * Uses native I2C command 0x2400 — zero external libraries required!
     */
    bool readSHT3x(uint8_t addr, float &tempC, float &humidity) {
        #if INTEGRALL_WIRE_AVAILABLE
        Wire.begin();
        Wire.beginTransmission(addr);
        Wire.write(0x24); // High repeatability, clock stretching disabled
        Wire.write(0x00);
        if (Wire.endTransmission() != 0) {
            tempC = -999.0f; humidity = -1.0f;
            return false;
        }
        delay(15);
        if (Wire.requestFrom(addr, (uint8_t)6) != 6) {
            tempC = -999.0f; humidity = -1.0f;
            return false;
        }
        uint16_t rawT = ((uint16_t)Wire.read() << 8) | Wire.read();
        Wire.read(); // CRC
        uint16_t rawH = ((uint16_t)Wire.read() << 8) | Wire.read();
        Wire.read(); // CRC
        tempC = -45.0f + (175.0f * ((float)rawT / 65535.0f));
        humidity = constrain(100.0f * ((float)rawH / 65535.0f), 0.0f, 100.0f);
        return true;
        #else
        (void)addr;
        tempC = -999.0f; humidity = -1.0f;
        return false;
        #endif
    }

    /**
     * Read UV Index (0.0 to 15.0) from GUVA-S12SD or ML8511 analog UV sensor
     */
    float readUVIndex(uint8_t pin) {
        float v = readPinVoltage(pin, 5);
        float uv = v / 0.1f; // GUVA-S12SD standard: ~0.1V per UV Index
        return constrain(uv, 0.0f, 15.0f);
    }

    /**
     * Read TCS3200 / TCS230 Color Sensor RGB frequencies (normalized 0-255)
     */
    void readColorRGB(uint8_t s2Pin, uint8_t s3Pin, uint8_t outPin, uint8_t &r, uint8_t &g, uint8_t &b) {
        pinMode(s2Pin, OUTPUT);
        pinMode(s3Pin, OUTPUT);
        pinMode(outPin, INPUT);

        // Red filter (S2=LOW, S3=LOW)
        digitalWrite(s2Pin, LOW); digitalWrite(s3Pin, LOW);
        unsigned long pR = pulseIn(outPin, LOW, 20000UL);

        // Green filter (S2=HIGH, S3=HIGH)
        digitalWrite(s2Pin, HIGH); digitalWrite(s3Pin, HIGH);
        unsigned long pG = pulseIn(outPin, LOW, 20000UL);

        // Blue filter (S2=LOW, S3=HIGH)
        digitalWrite(s2Pin, LOW); digitalWrite(s3Pin, HIGH);
        unsigned long pB = pulseIn(outPin, LOW, 20000UL);

        r = (pR == 0) ? 0 : (uint8_t)constrain(map((long)pR, 200, 20, 0, 255), 0, 255);
        g = (pG == 0) ? 0 : (uint8_t)constrain(map((long)pG, 200, 20, 0, 255), 0, 255);
        b = (pB == 0) ? 0 : (uint8_t)constrain(map((long)pB, 200, 20, 0, 255), 0, 255);
    }

    /**
     * Read Sharp GP2Y0A21YK0F Analog IR Distance Sensor (10cm to 80cm)
     */
    float readIRDistanceCM(uint8_t pin) {
        float v = readPinVoltage(pin, 5);
        if (v < 0.25f) return 80.0f;
        float cm = 29.988f * powf(v, -1.173f);
        return constrain(cm, 10.0f, 80.0f);
    }

    /**
     * Read VL53L0X Laser Time-of-Flight (ToF) Distance Sensor in millimeters
     */
    int readLaserDistanceMM() {
        #if INTEGRALL_VL53L0X_AVAILABLE
        if (!_tof) {
            _tof = new Adafruit_VL53L0X();
            if (!_tof->begin()) {
                delete _tof;
                _tof = nullptr;
                return -1;
            }
        }
        VL53L0X_RangingMeasurementData_t measure;
        _tof->rangingTest(&measure, false);
        if (measure.RangeStatus != 4) {
            return (int)measure.RangeMilliMeter;
        }
        return -1;
        #else
        return -1;
        #endif
    }

    /**
     * Read MQ-Series Gas Sensor concentration in PPM (MQ-2, MQ-4, MQ-7, MQ-135)
     * Defaults to MQ-135 CO2-equivalent curve (a=116.6, b=-2.769)
     */
    float readGasPPM(uint8_t pin, float r0 = 10.0f, float rl = 10.0f, float a = 116.602f, float b = -2.769f) {
        float v = readPinVoltage(pin, 5);
        float vRef = getAdcRefVoltage();
        if (v <= 0.01f || v >= vRef || r0 <= 0.0f || rl <= 0.0f) return 0.0f;
        float rs = rl * ((vRef - v) / v);
        float ratio = rs / r0;
        if (ratio <= 0.0f) return 0.0f;
        return a * powf(ratio, b);
    }

    /**
     * Read Sharp GP2Y1010AU0F Optical Dust / PM2.5 Sensor (returns µg/m³)
     */
    float readDustDensity(uint8_t ledPin, uint8_t analogPin) {
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW); // Activate IR LED (active-low on most GP2Y1010 breakout boards)
        delayMicroseconds(280);
        int raw = analogRead(analogPin);
        delayMicroseconds(40);
        digitalWrite(ledPin, HIGH);
        delayMicroseconds(9680);

        float v = ((float)raw * getAdcRefVoltage()) / (float)getAdcMax();
        float densityMgM3 = 0.17f * v - 0.1f;
        if (densityMgM3 < 0.0f) densityMgM3 = 0.0f;
        return densityMgM3 * 1000.0f; // µg/m³
    }

    /**
     * Read MH-Z19B / PWM NDIR CO2 Sensor (returns PPM, typically 400 to 5000, or -1 on timeout)
     */
    int readCO2PWM(uint8_t pwmPin, int rangePPM = 5000) {
        pinMode(pwmPin, INPUT);
        unsigned long thUs = pulseIn(pwmPin, HIGH, 2500000UL);
        if (thUs == 0) return -1;
        if (thUs <= 2000UL) return 0;
        if (thUs >= 1002000UL) return rangePPM;
        long ppm = (long)(((uint64_t)rangePPM * (thUs - 2000UL)) / 1000000UL);
        return (int)constrain(ppm, 0L, (long)rangePPM);
    }

    /**
     * Read TDS (Total Dissolved Solids / Water Quality) Sensor in PPM with temperature compensation
     */
    float readTDS(uint8_t pin, float waterTempC = 25.0f) {
        float v = readPinVoltage(pin, 10);
        float compCoeff = 1.0f + 0.02f * (waterTempC - 25.0f);
        if (compCoeff <= 0.0f) compCoeff = 1.0f;
        float compV = v / compCoeff;
        float tds = (133.42f * compV * compV * compV
                     - 255.86f * compV * compV
                     + 857.39f * compV) * 0.5f;
        return (tds < 0.0f) ? 0.0f : tds;
    }

    /**
     * Read Analog pH Sensor Probe (0.0 to 14.0 pH)
     */
    float readPH(uint8_t pin, float neutralVoltage = 2.50f, float slope = -5.70f) {
        float v = readPinVoltage(pin, 10);
        float ph = 7.0f + ((v - neutralVoltage) * slope);
        return constrain(ph, 0.0f, 14.0f);
    }

    /**
     * Read Water Turbidity Sensor in NTU (Nephelometric Turbidity Units, 0 to 3000)
     */
    float readTurbidityNTU(uint8_t pin) {
        float v = readPinVoltage(pin, 10);
        // Scale to 5V curve if running on 3.3V ADC with voltage divider
        if (v < 2.5f) return 3000.0f;
        if (v > 4.2f) return 0.0f;
        float ntu = -1120.4f * v * v + 5742.3f * v - 4352.9f;
        return constrain(ntu, 0.0f, 3000.0f);
    }

    /**
     * Read Water Flow Sensor (YF-S201 / Hall-effect turbine) in Liters Per Minute (L/min)
     * @param pin                Digital pulse pin
     * @param calibrationFactor  Pulses per second per L/min (7.5 for YF-S201 1/2" sensor)
     */
    float readWaterFlowLPM(uint8_t pin, float calibrationFactor = 7.5f) {
        if (calibrationFactor <= 0.0f) return 0.0f;
        pinMode(pin, INPUT_PULLUP);
        unsigned long highUs = pulseIn(pin, HIGH, 250000UL);
        unsigned long lowUs  = pulseIn(pin, LOW, 250000UL);
        unsigned long periodUs = highUs + lowUs;
        if (periodUs == 0) return 0.0f;
        float freqHz = 1000000.0f / (float)periodUs;
        return freqHz / calibrationFactor;
    }

    /**
     * Read HX711 24-Bit Load Cell Amplifier (Raw 24-bit signed value)
     * Uses native bit-banged protocol — zero external libraries required!
     */
    long readWeightRaw(uint8_t doutPin, uint8_t sckPin, uint32_t timeoutMs = 200) {
        pinMode(sckPin, OUTPUT);
        pinMode(doutPin, INPUT);
        digitalWrite(sckPin, LOW);

        unsigned long start = millis();
        while (digitalRead(doutPin) == HIGH) {
            if (millis() - start > timeoutMs) return 0;
        }

        uint32_t value = 0;
        for (uint8_t i = 0; i < 24; i++) {
            digitalWrite(sckPin, HIGH);
            delayMicroseconds(1);
            value = (value << 1) | (digitalRead(doutPin) ? 1UL : 0UL);
            digitalWrite(sckPin, LOW);
            delayMicroseconds(1);
        }

        // 25th pulse sets Channel A, Gain 128 for next conversion
        digitalWrite(sckPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(sckPin, LOW);

        if (value & 0x800000UL) {
            value |= 0xFF000000UL; // Sign-extend 24-bit two's complement to 32-bit
        }
        return (long)((int32_t)value);
    }

    /**
     * Read Calibrated Weight in Grams from HX711 Load Cell
     */
    float readWeightGrams(uint8_t doutPin, uint8_t sckPin, long tareOffset = 0, float scaleFactor = 420.0f) {
        if (scaleFactor == 0.0f) return 0.0f;
        long raw = readWeightRaw(doutPin, sckPin);
        return (float)(raw - tareOffset) / scaleFactor;
    }

    /**
     * Read ACS712 Hall-Effect DC Current Sensor in Amps
     * @param sensitivityMvPerA  185.0 (5A module), 100.0 (20A module), or 66.0 (30A module)
     */
    float readCurrentDC(uint8_t pin, float sensitivityMvPerA = 185.0f, float zeroPointV = -1.0f) {
        if (sensitivityMvPerA <= 0.0f) return 0.0f;
        if (zeroPointV < 0.0f) zeroPointV = getAdcRefVoltage() * 0.5f;
        float v = readPinVoltage(pin, 10);
        return ((v - zeroPointV) * 1000.0f) / sensitivityMvPerA;
    }

    /**
     * Read ACS712 Hall-Effect AC RMS Current in Amps (samples peak-to-peak over 20ms 50/60Hz cycle)
     */
    float readCurrentAC(uint8_t pin, float sensitivityMvPerA = 185.0f, uint16_t windowMs = 20) {
        if (sensitivityMvPerA <= 0.0f) return 0.0f;
        int minVal = getAdcMax();
        int maxVal = 0;
        unsigned long start = millis();
        do {
            int val = analogRead(pin);
            if (val > maxVal) maxVal = val;
            if (val < minVal) minVal = val;
        } while (millis() - start < windowMs);

        float vpp = ((float)(maxVal - minVal) * getAdcRefVoltage()) / (float)getAdcMax();
        float vRms = (vpp * 0.5f) * 0.70710678f;
        return (vRms * 1000.0f) / sensitivityMvPerA;
    }

    /**
     * Read DC Voltage Sensor (0-25V resistor divider module, default 5:1 ratio)
     */
    float readVoltageDivider(uint8_t pin, float dividerRatio = 5.0f) {
        return readPinVoltage(pin, 5) * dividerRatio;
    }

    /**
     * Read Analog Pulse / Heart-Rate PPG Sensor (normalized 0-100%)
     */
    int readHeartPulsePercent(uint8_t pin) {
        return readAnalogPercent(pin);
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
     * Read BMP280 Pressure & Temperature (or BME280 Pressure) in hPa
     */
    float readPressure(uint8_t addr = 0x76) {
        #if INTEGRALL_BME280_AVAILABLE
        float t, h, p;
        readEnvironment(addr, t, h, p);
        if (p > 0.0f) return p;
        #endif
        #if INTEGRALL_BMP280_AVAILABLE
        if (!_bmp || _bmpAddr != addr || !_bmpReady) {
            if (!_bmp) _bmp = new Adafruit_BMP280();
            _bmpReady = _bmp->begin(addr);
            _bmpAddr = addr;
            if (!_bmpReady) return -1.0f;
        }
        return _bmp->readPressure() / 100.0f;
        #else
        (void)addr;
        return -1.0f;
        #endif
    }

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
    /**
     * Get Pitch and Roll angles in degrees (-180 to +180) and die temperature (°C) from MPU6050
     */
    float getPitch() {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (!_imuStarted) return 0.0f;
        sensors_event_t a, g, temp;
        _mpu.getEvent(&a, &g, &temp);
        return atan2f(-a.acceleration.x, sqrtf(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0f / PI;
        #else
        return 0.0f;
        #endif
    }

    float getRoll() {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (!_imuStarted) return 0.0f;
        sensors_event_t a, g, temp;
        _mpu.getEvent(&a, &g, &temp);
        return atan2f(a.acceleration.y, a.acceleration.z) * 180.0f / PI;
        #else
        return 0.0f;
        #endif
    }

    float getIMUTemperature() {
        #if INTEGRALL_MPU6050_AVAILABLE
        if (!_imuStarted) return -999.0f;
        sensors_event_t a, g, temp;
        _mpu.getEvent(&a, &g, &temp);
        return temp.temperature;
        #else
        return -999.0f;
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

#if INTEGRALL_BMP280_AVAILABLE
    Adafruit_BMP280* _bmp = nullptr;
    uint8_t _bmpAddr = 0;
    bool _bmpReady = false;
#endif

#if INTEGRALL_VL53L0X_AVAILABLE
    Adafruit_VL53L0X* _tof = nullptr;
#endif

#if INTEGRALL_MPU6050_AVAILABLE
    Adafruit_MPU6050 _mpu;
    bool _imuStarted = false;
#endif
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_SENSORS_ENABLED || INTEGRALL_MODULE_IMU_ENABLED
#endif // INTEGRALL_SENSOR_MODULE_H
