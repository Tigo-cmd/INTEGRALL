/**
 * SmartCamera.ino - Professional ESP32-CAM implementation
 * 
 * The "Integrall" Way:
 * 1. Define Camera Support
 * 2. Set credentials in ONE line
 * 3. Start Camera in ONE line
 */

#define INTEGRALL_ENABLE_WIFI
#define INTEGRALL_ENABLE_CAMERA
#define INTEGRALL_DEBUG_LEVEL 3 // Enable INFO logging for debugging
#include <Integrall.h>

Integrall::System integrall;

void setup() {
    // 1. One-line initialization (No editing header files!)
    integrall.begin("YourWiFiName", "YourWiFiPassword");

    // 2. One-line Camera Setup (Hides 50 lines of pin config)
    if (integrall.enableCamera()) {
        Serial.println(">> Integrall: Camera Ready!");
        Serial.print(">> Stream URL: ");
        Serial.println(integrall.getCameraStreamURL());
    } else {
        Serial.println(">> Integrall: Camera Init Failed.");
    }
}

void loop() {
    // 3. Keep the system alive (WiFi recovery, Cloud commands)
    integrall.handle();
}
