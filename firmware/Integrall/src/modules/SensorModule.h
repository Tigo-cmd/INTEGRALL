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
     */
    float readDistanceCM(uint8_t trigPin, uint8_t echoPin) {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
        
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
        if (duration == 0) return -1; // Out of range
        
        return duration * 0.034 / 2;
    }

    /**
     * Helper to check if a simple digital sensor is triggered (e.g., PIR Motion)
     */
    bool isTriggered(uint8_t pin, bool activeHigh = true) {
        pinMode(pin, INPUT);
        bool state = digitalRead(pin);
        return activeHigh ? state : !state;
    }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_SENSORS_ENABLED
#endif // INTEGRALL_SENSOR_MODULE_H
