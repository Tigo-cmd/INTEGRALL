/**
 * SensorModule.h
 * 
 * Simple wrapper for common sensors (DHT11/22, HC-SR04, Analog)
 * Hides timing logic, pulse measuring, and boilerplate math.
 */

#ifndef INTEGRALL_SENSOR_MODULE_H
#define INTEGRALL_SENSOR_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_SENSORS_ENABLED

#include <Arduino.h>

// Optional: include DHT support if user has the library installed
// Install via Library Manager: "DHT sensor library" by Adafruit
#if __has_include(<DHT.h>)
  #include <DHT.h>
  #define INTEGRALL_DHT_AVAILABLE 1
#else
  #define INTEGRALL_DHT_AVAILABLE 0
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
        return map(val, 0, 1023, 0, 100);
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
     * Helper to check if a simple digital sensor is triggered (e.g., PIR Motion)
     */
    bool isTriggered(uint8_t pin, bool activeHigh = true) {
        pinMode(pin, INPUT);
        bool state = digitalRead(pin);
        return activeHigh ? state : !state;
    }

#if INTEGRALL_DHT_AVAILABLE
    /**
     * Read temperature from DHT11/DHT22 sensor
     * @param pin     Data pin
     * @param type    DHT11, DHT22, or DHT21
     * @param celsius true = Celsius, false = Fahrenheit
     */
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
#endif
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_SENSORS_ENABLED
#endif // INTEGRALL_SENSOR_MODULE_H
