/**
 * DistanceAlerts.ino - Professional distance-based warning system
 * 
 * Replaces complex nested if-else and delay() logic with 
 * clean Integrall one-liners.
 * 
 * Features:
 *   - Green/Yellow/Red LED distance profiling.
 *   - Proactive buzzer alerts.
 *   - Automatic Dashboard status syncing.
 */

#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_BUZZER
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>

Integrall::System integrall;
int green, yellow, red;

void setup() {
    integrall.begin();
    
    // 1. Setup outputs as relays (this makes them appear on your phone/web!)
    green  = integrall.enableRelay(11, false, "Green LED");
    yellow = integrall.enableRelay(10, false, "Yellow LED");
    red    = integrall.enableRelay(9,  false, "Red LED");
    
    integrall.enableBuzzer(3);
}

void loop() {
    integrall.handle(); // Keeps background tasks (WiFi/Sync) active

    // 2. Measure distance (internal noise filtering included)
    float dist = integrall.readDistance(6, 7);

    // 3. Clean threshold mapping + Optional Alerts
    integrall.setRelay(green,  dist < 50 && dist > 0);
    integrall.setRelay(yellow, dist < 20 && dist > 0);
    
    // The 'true' flag here automatically triggers the buzzer alert 
    // when the relay is ON, and handles all timing for you!
    integrall.setRelay(red,    dist < 5  && dist > 0, true);
}
