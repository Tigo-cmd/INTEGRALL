/**
 * ArduinoOfflineTest.ino - Testing Integrall on Standard Boards
 * 
 * Works on: Arduino Uno, Mega, Nano, Pro Mini.
 * Use this to verify non-blocking blinking and buzzer patterns
 * using only the Serial Monitor.
 */

#define INTEGRALL_ENABLE_BUZZER
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>

Integrall::System integrall;
int virtualRelay;

void setup() {
    // Starts Serial at 115200 automatically
    integrall.begin();
    
    Serial.println("\n***********************************");
    Serial.println("*    INTEGRALL OFFLINE SANDBOX    *");
    Serial.println("***********************************");
    
    // Setup Pin 13 (Standard LED) and Pin 3 (Buzzer)
    virtualRelay = integrall.enableRelay(13, false, "Status LED");
    integrall.enableBuzzer(3);
    
    // Start a non-blocking sequence
    Serial.println("[SYSTEM] Playing Startup Success Melody...");
    integrall.buzzerSuccess();
    
    Serial.println("[SYSTEM] Starting non-blocking blinker on Pin 13...");
    integrall.blink(13, 1000);
}

void loop() {
    // CRITICAL: Handle the background timing state machine
    integrall.handle();

    // Simple interactive check
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 5000) {
        lastLog = millis();
        Serial.print("[HEARTBEAT] System Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds.");
        
        // Trigger a recurring non-blocking chirp
        integrall.buzzerBeep(100);
    }
}
