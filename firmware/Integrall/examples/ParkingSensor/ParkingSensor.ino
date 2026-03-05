/**
 * ParkingSensor.ino - Garage Distance Finder
 * 
 * Uses:
 *   - HC-SR04 Ultrasonic Sensor => to measure distance
 *   - LCD (16x2)                => to show distance & warning
 *   - Buzzer                    => for audible STOP alert
 * 
 * Wiring:
 *   Trig Pin     -> GPIO 5
 *   Echo Pin     -> GPIO 18
 *   Buzzer       -> GPIO 25
 *   LCD          -> GPIO 21, 22
 */

#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_LCD
#define INTEGRALL_ENABLE_BUZZER

#include <Integrall.h>

Integrall::System integrall;

void setup() {
    integrall.begin();
    integrall.enableBuzzer(25);

    // 1. Configure the Parking Sensor
    // parkingSetup(trig, echo, warningCM, stopCM)
    integrall.parkingSetup(5, 18, 60.0, 20.0);
}

void loop() {
    // 2. Integrall reads the sensor 2x per second, filters noise,
    // and updates the LCD with "Safe", "Slow down", or "STOP!".
    integrall.parkingUpdate();
    
    integrall.handle();
}
