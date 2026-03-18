/**
 * WeatherStation.ino - Smart Temperature/Humidity Monitor
 * 
 * Uses:
 *   - DHT22 Sensor   => for temp & humidity
 *   - LCD (16x2)     => for desktop display
 * 
 * Integration:
 *   Sends data to the Integrall Backend every 10 seconds.
 * 
 * Wiring:
 *   DHT Data Pin -> GPIO 4
 *   LCD I2C      -> Standard SDA/SCL pins (e.g. 21/22 on ESP32)
 */

#define INTEGRALL_DEBUG_LEVEL 3
#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_LCD

#include <Integrall.h>

Integrall::System integrall;

void setup() {
    Serial.begin(115200);
    delay(1000); // Give the serial monitor time to connect

    // Note: To send to cloud, add DeviceConfig with WiFi details in begin()
    integrall.begin();

    // 1. Configure the Weather Station
    // weatherSetup(pin, dhtType, intervalSeconds)
    integrall.weatherSetup(4, 22, 5); 
}

void loop() {
    // 2. Integrall reads the DHT, formats the LCD, and
    // auto-logs the data to the web dashboard.
    integrall.weatherUpdate();
    
    integrall.handle();
}
