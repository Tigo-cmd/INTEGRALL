#define INTEGRALL_ENABLE_OLED
#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_INPUT
#define INTEGRALL_ENABLE_WIFI
#include <Integrall.h>

/**
 * INTEGRALL WEATHER STATION - FULL REDUNDANCY CLOUD VERSION
 * Syncs BME280, DHT11, LDR, and Rain sensors + Derived Math.
 */

// PIN Definitions
#define ADDR_BME280 0x76
#define PIN_DHT     27
#define PIN_BUTTON  32
#define PIN_LDR     35
#define PIN_RAIN    34

// Network Config
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* BACKEND_URL = "http://192.168.1.100:5000/telemetry";

Integrall::System integrall;

// Variable Storage
float tempBME, humBME, pres, alt;
float tempDHT, humDHT;
float dewPoint, heatIndex;
int light, rain;
int screen = 0;

void setup() {
    integrall.begin(WIFI_SSID, WIFI_PASS);
    readSensors();
}

void loop() {
    integrall.handle();
    
    // Sync to Cloud every 10 seconds
    static uint32_t lastCloudSync = 0;
    if (millis() - lastCloudSync > 10000) {
        readSensors();
        
        if (integrall.isWiFiConnected()) {
            StaticJsonDocument<512> data;
            // Primary Sensors (BME)
            data["temp_bme"] = tempBME;
            data["hum_bme"]  = humBME;
            data["pressure"] = pres;
            data["altitude"] = alt;
            
            // Secondary Sensors (DHT)
            data["temp_dht"] = tempDHT;
            data["hum_dht"]  = humDHT;
            
            // Derived Math
            data["dew_point"]  = dewPoint;
            data["heat_index"] = heatIndex;
            
            // Environment
            data["light_level"] = light;
            data["rain_level"]  = rain;
            data["uptime"]      = millis() / 1000;
            
            // Use generic HTTP Post to target our exact Python endpoint
            String payload;
            serializeJson(data, payload);
            integrall.httpPost(BACKEND_URL, payload.c_str());
        }
        lastCloudSync = millis();
    }
    
    if (integrall.inputButtonPressed(PIN_BUTTON)) {
        screen = (screen + 1) % 4;
        integrall.oledClear();
    }
    
    updateUI();
}

void readSensors() {
    // 1. Read BME280
    integrall.readEnvironment(ADDR_BME280, tempBME, humBME, pres);
    alt = integrall.calculateAltitude(pres);

    // 2. Read DHT11
    tempDHT = integrall.readTemperature(PIN_DHT, DHT11);
    humDHT = integrall.readHumidity(PIN_DHT, DHT11);

    // 3. Native Math (Calculated Helpers)
    dewPoint = integrall.calculateDewPoint(tempBME, humBME);
    heatIndex = integrall.calculateHeatIndex(tempBME, humBME);

    // 4. Analog 
    light = integrall.readLightPercent(PIN_LDR, true);
    rain = integrall.readAnalogPercent(PIN_RAIN);
}

void updateUI() {
    // OLED visualization logic remains the same but with redunant data
    switch (screen) {
        case 0: 
            integrall.oledPrint("BME vs DHT", 0, 0);
            integrall.oledPrintValue("BME: ", tempBME, 2);
            integrall.oledPrintValue("DHT: ", tempDHT, 3);
            break;
        case 1: 
            integrall.oledPrint("ATMOSPHERE", 0, 0);
            integrall.oledPrintValue("Pres: ", pres, 2);
            integrall.oledPrintValue("Alt: ", alt, 4);
            break;
        case 2: 
            integrall.oledPrint("ENVIRONMENT", 0, 0);
            integrall.oledPrintValue("Light: ", light, 2);
            integrall.oledPrintValue("Rain: ", rain, 3);
            break;
        case 3: 
            integrall.oledPrint("CALCULATED", 0, 0);
            integrall.oledPrintValue("DewPt: ", dewPoint, 2);
            integrall.oledPrintValue("HeatIdx: ", heatIndex, 3);
            break;
    }
}
