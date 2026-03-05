/**
 * AlarmSystem.ino - DIY Smart Security System
 * 
 * Uses:
 *   - PIR Motion Sensor  => to detect intruders
 *   - Relay Module       => to trigger a siren/strobe
 *   - Active Buzzer      => for audible alerts
 *   - LCD (16x2)         => for status display
 * 
 * Wiring:
 *   PIR Signal   -> GPIO 19
 *   Relay        -> GPIO 13
 *   Buzzer       -> GPIO 25
 *   LCD SDA/SCL  -> GPIO 21, 22
 */

#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_BUZZER
#define INTEGRALL_ENABLE_LCD

#include <Integrall.h>

Integrall::System integrall;

void setup() {
    integrall.begin();

    // 1. Enable hardware
    int siren = integrall.enableRelay(13, true, "Siren");
    integrall.enableBuzzer(25);

    // 2. Configure the Alarm (SensorPin, RelayIndex, CooldownMs)
    integrall.alarmSetup(19, siren, 15000); // 15s cooldown between alerts
}

void loop() {
    // 3. Let Integrall handle the detection, LCD text, and buzzer beeps!
    integrall.alarmUpdate();
    
    // Always call handle() to keep network tasks alive
    integrall.handle();
}
