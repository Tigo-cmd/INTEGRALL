/**
 * SerialOnlyTest.ino - Testing Integrall without any hardware
 * 
 * This sketch demonstrates how Integrall handles the backend, 
 * identity, and logging using only the Serial Monitor.
 */

#include <Integrall.h>

Integrall::System integrall;

void setup() {
    // Zero-config identification and serial initialization
    integrall.begin();
}

void loop() {
    // Handle background polling and WiFi reconnection
    integrall.handle();

    // Periodic Virtual Telemetry
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 10000) {
        lastUpdate = millis();

        // Collect internal system data
        StaticJsonDocument<256> doc;
        doc["uptime"] = millis() / 1000;
        doc["ip"] = integrall.getIPAddress();
        doc["signal"] = integrall.getWiFiStrength();
        doc["status"] = integrall.getStatusString();
        doc["free_heap"] = ESP.getFreeHeap();

        // Print pretty JSON to Serial
        Serial.println("\n[SYSTEM REPORT]");
        serializeJsonPretty(doc, Serial);
        Serial.println();

        // Forward report to the Integrall Backend
        if (integrall.isOnline()) {
            integrall.sendTelemetry(doc);
            Serial.println(">> Telemetry synchronized with cloud.");
        } else {
            Serial.println(">> (Offline) Telemetry cached locally.");
        }
    }
}
