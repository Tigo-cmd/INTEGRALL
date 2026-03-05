/**
 * DriverSafety.ino - Using Integrall for Real-Time Safety
 * 
 * Replaces blocking delay() logic with smooth, responsive 
 * non-blocking timers.
 * 
 * Logic:
 *   1. Monitor IR Sensor (Eye detection).
 *   2. If eyes closed for 2s => Chirp warning.
 *   3. If eyes closed for 4s => Kill motor and sound Siren.
 */

#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_BUZZER
#define INTEGRALL_ENABLE_SENSORS
#include <Integrall.h>

Integrall::System integrall;
int motor;
unsigned long closedTime = 0;

void setup() {
    integrall.begin();
    
    // 1. Setup hardware with professional naming
    motor = integrall.enableRelay(8, false, "Engine");
    integrall.enableBuzzer(9);
    
    integrall.relayOn(motor); // Start engine initially
}

void loop() {
    // 2. Process background tasks (WiFi, Cloud, Timers)
    integrall.handle();

    // 3. Main Safety Logic (No delays!)
    bool eyesClosed = !integrall.isTriggered(2); // Check sensor on Pin 2

    if (eyesClosed) {
        if (closedTime == 0) closedTime = millis();
        unsigned long duration = millis() - closedTime;

        if (duration >= 4000) {
            // CRITICAL: Stop motor and play triple-beep alert
            integrall.relayOff(motor);    
            integrall.buzzerAlert();      
        } 
        else if (duration >= 2000) {
            // WARNING: Short warning beep
            integrall.buzzerBeep(100);    
        }
    } 
    else {
        // Eyes are open, reset everything
        closedTime = 0;
        integrall.buzzerOff();
        
        // Restart motor if it was safely cut
        if (!integrall.relayIsOn(motor)) {
            integrall.relayOn(motor);
        }
    }
}
