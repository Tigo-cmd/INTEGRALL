#define INTEGRALL_ENABLE_OLED
#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_INPUT
#define INTEGRALL_ENABLE_WIFI
#include "firmware/Integrall/src/Integrall.h"

// PIN Definitions
#define ADDR_BME280 0x76
#define PIN_DHT     27
#define TYPE_DHT    DHT11
#define PIN_LDR     35
#define PIN_RAIN    34
#define PIN_BUTTON  32

// System Instance
Integrall::System integrall;

// Global Data
float temp, hum, pres;
int light, rain;
int screen = 0;

void setup() {
    // MODE CHOICE:
    // Option A: Station Mode (Connect to your home router to send data to cloud)
    // integrall.begin("HomeWiFi", "Pass123", "http://your-backend.com");
    
    // Option B: Access Point Mode (ESP32 creates its own hotspot)
    integrall.beginAP("Integrall-Weather");

    // Initial sensor read
    readSensors();
}

void loop() {
    integrall.handle();
    
    // Update sensors every 5 seconds
    static uint32_t lastRead = 0;
    if (millis() - lastRead > 5000) {
        readSensors();
        
        // --- CLOUD SYNC ---
        // Automatically syncs to backend if initialized in Station Mode
        if (integrall.isOnline()) {
            StaticJsonDocument<256> doc;
            doc["temp"] = temp;
            doc["hum"] = hum;
            doc["pres"] = pres;
            doc["light"] = light;
            doc["rain"] = rain;
            integrall.sendTelemetry(doc);
        }
        
        lastRead = millis();
    }
    
    // Handle Input
    if (integrall.inputButtonPressed(PIN_BUTTON)) {
        screen = (screen + 1) % 4;
        integrall.oledClear();
    }
    
    updateUI();
}

void readSensors() {
    integrall.readEnvironment(ADDR_BME280, temp, hum, pres);
    light = integrall.readLightPercent(PIN_LDR, true);
    rain = integrall.readAnalogPercent(PIN_RAIN);
}

void updateUI() {
    switch (screen) {
        case 0: // Temp & Hum
            integrall.oledPrint("TEMP & HUMIDITY", 0, 0);
            integrall.oledPrintValue("Temp: ", temp, 2);
            integrall.oledPrintValue("Hum : ", hum, 3);
            break;
            
        case 1: // Pressure
            integrall.oledPrint("PRESSURE DATA", 0, 0);
            integrall.oledPrintValue("Pres: ", pres, 2);
            break;
            
        case 2: // Environment
            integrall.oledPrint("ENVIRONMENT", 0, 0);
            integrall.oledPrintValue("Light %: ", light, 2);
            integrall.oledPrintValue("Rain %: ", rain, 3);
            integrall.oledPrint(rain > 50 ? "Rain: YES" : "Rain: NO", 0, 5);
            break;
            
        case 3: // System status
            integrall.oledPrint("SYSTEM STATUS", 0, 0);
            integrall.oledPrint("Cloud: ", 0, 2);
            integrall.oledPrint(integrall.isOnline() ? "ONLINE" : "OFFLINE", 7, 2);
            integrall.oledPrint("IP: ", 0, 3);
            integrall.oledPrint(integrall.getIPAddress(), 4, 3);
            break;
    }
}
