/**
 * BasicRelay.ino
 * 
 * Simplest possible Integrall example
 * Control a relay over WiFi with backend integration
 * 
 * Hardware needed:
 * - ESP32 dev board
 * - Relay module on GPIO 5
 * (Optional) LED on GPIO 2 for status indication
 * 
 * Setup time: < 10 minutes
 * Code: < 20 lines
 */

// Enable only the relay module to save memory
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_DEBUG_LEVEL 2  // Warnings and errors

#include <Integrall.h>

// Configuration (in production, use WiFiManager or secure storage)
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* BACKEND_URL = "http://192.168.1.100:8000";  // Your computer's IP
const char* API_KEY = "your-secret-api-key-here";

// Create Integrall system
Integrall::System integrall;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== Integrall BasicRelay Starting ===");
    
    // Configure device
    Integrall::DeviceConfig config;
    config.wifi_ssid = WIFI_SSID;
    config.wifi_password = WIFI_PASSWORD;
    config.backend_url = BACKEND_URL;
    config.api_key = API_KEY;
    config.poll_interval_ms = 3000;  // Check for commands every 3 seconds
    
    // Initialize Integrall
    if (!integrall.begin(config)) {
        Serial.println("Failed to initialize Integrall!");
        Serial.println("Check WiFi credentials and backend URL");
        return;
    }
    
    // Enable relay on GPIO 5 (active-low, common for optocoupled modules)
    int relay = integrall.enableRelay(5, true, "Main Relay");
    if (relay < 0) {
        Serial.println("Failed to initialize relay!");
    } else {
        Serial.println("Relay ready on GPIO 5");
        
        // Optional: Set safety timeout (auto-off after 30 seconds)
        integrall.relaySetTimeout(relay, 30000);
        
        // Optional: Attach physical button on GPIO 4
        // integrall.relayAttachButton(relay, 4, true);
    }
    
    Serial.println("Setup complete. Device should be online shortly.");
    Serial.print("Device ID: ");
    Serial.println(integrall.getDeviceId());
}

void loop() {
    // Handle all Integrall operations (WiFi, backend, relay updates)
    integrall.handle();
    
    // Your custom logic here
    // Example: Blink status LED when online
    static unsigned long lastBlink = 0;
    if (integrall.isOnline() && millis() - lastBlink > 1000) {
        lastBlink = millis();
        digitalWrite(2, !digitalRead(2));  // Toggle built-in LED
    }
    
    // Small delay to prevent watchdog issues
    delay(10);
}
