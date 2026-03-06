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

        Serial.println("\n[SYSTEM REPORT]");
        Serial.print("Uptime: "); Serial.println(millis() / 1000);
        Serial.print("IP: "); Serial.println(integrall.getIPAddress());
        Serial.print("Status: "); Serial.println(integrall.getStatusString());

        // JSON and Telemetry are only available on IoT boards (ESP32/ESP8266)
        #if defined(ESP32) || defined(ESP8266)
        StaticJsonDocument<256> doc;
        doc["uptime"] = millis() / 1000;
        doc["ip"] = integrall.getIPAddress();
        doc["status"] = integrall.getStatusString();
        doc["free_heap"] = ESP.getFreeHeap();

        serializeJsonPretty(doc, Serial);
        Serial.println();

        if (integrall.isOnline()) {
            integrall.sendTelemetry(doc);
            Serial.println(">> Telemetry synchronized with cloud.");
        }
        #else
        Serial.println(">> (Offline) Cloud features disabled for this board.");
        #endif
    }
}
