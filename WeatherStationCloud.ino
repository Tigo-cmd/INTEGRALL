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
    Adafruit_SSD1306* d = integrall.oled();
    if (!d) return;

    d->clearDisplay();
    d->setTextColor(SSD1306_WHITE);

    // Draw a top header bar on every screen
    d->fillRect(0, 0, 128, 12, SSD1306_WHITE);
    d->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    d->setCursor(2, 2);
    
    switch (screen) {
        case 0: {
            d->print("WEATHER STATION ");
            d->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            
            // Big Temperature
            d->setTextSize(3);
            d->setCursor(5, 20);
            d->print(tempBME, 1);
            
            // Degree symbol manually
            d->drawCircle(82, 22, 3, SSD1306_WHITE);
            d->setTextSize(1);
            d->setCursor(88, 20);
            d->print("C");
            
            // Humidity below
            d->setTextSize(1);
            d->setCursor(5, 50);
            d->print("Hum: ");
            d->print(humBME, 1);
            d->print("%");
            break;
        }
        case 1: {
            d->print("ATMOSPHERE INFO");
            d->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            
            d->setTextSize(1);
            d->setCursor(0, 20);
            d->print("Pres: ");
            d->print(pres, 1);
            d->println(" hPa");
            
            d->setCursor(0, 32);
            d->print("Alt : ");
            d->print(alt, 0);
            d->println(" m");

            d->setCursor(0, 44);
            d->print("DewP: ");
            d->print(dewPoint, 1);
            d->println(" C");
            
            d->setCursor(0, 56);
            d->print("Heat: ");
            d->print(heatIndex, 1);
            d->println(" C");
            break;
        }
        case 2: {
            d->print("ENVIRONMENT");
            d->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            d->setTextSize(1);
            
            // Light Bar
            d->setCursor(0, 18);
            d->print("Light:");
            d->drawRect(40, 16, 80, 10, SSD1306_WHITE);
            d->fillRect(42, 18, map(light, 0, 100, 0, 76), 6, SSD1306_WHITE);

            // Rain Bar
            d->setCursor(0, 34);
            d->print("Rain: ");
            d->drawRect(40, 32, 80, 10, SSD1306_WHITE);
            int mappedRain = map(rain, 0, 100, 0, 76);
            if (mappedRain < 0) mappedRain = 0;
            d->fillRect(42, 34, mappedRain, 6, SSD1306_WHITE);

            // Redundancy Status
            d->setCursor(0, 52);
            d->print("DHT Backup: ");
            if (tempDHT > -50) d->print("OK"); else d->print("FAIL");
            break;
        }
        case 3: {
            d->print("SYSTEM STATUS");
            d->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            d->setTextSize(1);

            d->setCursor(0, 20);
            if (integrall.isWiFiConnected()) {
                d->print("WiFi: CONNECTED");
                d->setCursor(0, 35);
                d->print("IP: ");
                d->print(integrall.getIPAddress());
                
                // Draw cool wifi waves
                d->drawArc(110, 40, 4, 180, 270, SSD1306_WHITE);
                d->drawArc(110, 40, 8, 180, 270, SSD1306_WHITE);
                d->fillCircle(110, 40, 2, SSD1306_WHITE);
            } else {
                d->print("WiFi: SEARCHING...");
            }
            break;
        }
    }
    
    // Draw screen indicator dots at the bottom
    for (int i=0; i<4; i++) {
        if(i == screen) {
            d->fillRect(50 + (i*8), 60, 6, 4, SSD1306_WHITE);
        } else {
            d->drawRect(50 + (i*8), 60, 6, 4, SSD1306_WHITE);
        }
    }

    d->display();
}
