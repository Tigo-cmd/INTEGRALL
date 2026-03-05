/**
 * SmartSwitch.ino - Motion Activated Light
 * 
 * Uses:
 *   - PIR Motion Sensor  => to detect people
 *   - Relay Module       => to switch the 110V/220V light
 * 
 * Feature:
 *   When motion is detected, light turns ON.
 *   If no motion for 60 seconds, light turns OFF automatically.
 * 
 * Wiring:
 *   PIR Pin      -> GPIO 19
 *   Relay Pin    -> GPIO 13
 */

#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_RELAY

#include <Integrall.h>

Integrall::System integrall;

void setup() {
    integrall.begin();

    // 1. Enable relay
    int light = integrall.enableRelay(13, true, "LivingRoom");

    // 2. Configure the Smart Switch
    // smartSwitchSetup(relayIndex, pirPin, autoOffSeconds)
    integrall.smartSwitchSetup(light, 19, 60);
}

void loop() {
    // 3. Integrall handles the PIR trigger and the auto-off timer.
    integrall.smartSwitchUpdate();
    
    integrall.handle();
}
