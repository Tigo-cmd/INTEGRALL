/**
 * SmartCamera.ino - Professional ESP32-CAM implementation
 * 
 * The "Integrall" Way:
 * 1. Define Camera Support
 * 2. Set credentials in ONE line
 * 3. Start Camera in ONE line
 */

#define INTEGRALL_ENABLE_CAMERA
#include <Integrall.h>

Integrall::System integrall;

void setup() {
    // 1. One-line initialization (No editing header files!)
    integrall.begin("YourWiFiName", "YourWiFiPassword", "http://192.168.1.100:8000");

    // 2. One-line Camera Setup (Hides 50 lines of pin config)
    if (integrall.enableCamera()) {
        Serial.println(">> Integrall: Camera Ready!");
        integrall.buzzerSuccess();
    } else {
        Serial.println(">> Integrall: Camera Init Failed.");
    }
}

void loop() {
    // 3. Keep the system alive (WiFi recovery, Cloud commands)
    integrall.handle();
}
