/**
 * FullDemo.ino
 * 
 * Comprehensive Integrall Framework demonstration
 * Shows multiple modules working together:
 * - WiFi + Backend connection
 * - Relay control with safety features
 * - OLED display status
 * - Keypad security lock
 * - Telemetry reporting
 * 
 * Hardware Requirements:
 * - ESP32 DevKit
 * - 128x64 OLED Display (SSD1306, I2C)
 * - 4-channel Relay Module (optocoupled, active-low)
 * - 4x4 Matrix Keypad
 * - Jumper wires
 * 
 * Wiring Diagram:
 * 
 * OLED Display (I2C):
 *   VCC → 3.3V
 *   GND → GND
 *   SDA → GPIO 21 (default I2C SDA)
 *   SCL → GPIO 22 (default I2C SCL)
 * 
 * Relay Module:
 *   VCC → 5V (external power recommended for multiple relays)
 *   GND → GND
 *   IN1 → GPIO 5
 *   IN2 → GPIO 18
 *   IN3 → GPIO 19
 *   IN4 → GPIO 23
 * 
 * Keypad 4x4:
 *   R1 → GPIO 13
 *   R2 → GPIO 12
 *   R3 → GPIO 14
 *   R4 → GPIO 27
 *   C1 → GPIO 26
 *   C2 → GPIO 25
 *   C3 → GPIO 33
 *   C4 → GPIO 32
 * 
 * Reference Images:
 * - Relay wiring: https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2019/12/relay-esp32-wiring.png
 * - OLED wiring: https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2019/05/ESP8266_oled_display_wiring.png
 * - Keypad wiring: https://www.electronicwings.com/storage/PlatformSection/TopicContent/445/description/4x4%20Keypad%20Interfacing%20with%20ESP32.jpg
 */

// Enable all modules for this demo
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_OLED
#define INTEGRALL_ENABLE_KEYPAD
#define INTEGRALL_DEBUG_LEVEL 3  // Info level for detailed logging

#include <Integrall.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuration
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* BACKEND_URL = "http://192.168.1.100:8000";
const char* API_KEY = "your-secret-api-key";

// Pin definitions
#define RELAY_1_PIN     5
#define RELAY_2_PIN     18
#define RELAY_3_PIN     19
#define RELAY_4_PIN     23

#define KEYPAD_ROWS     4
#define KEYPAD_COLS     4
byte rowPins[KEYPAD_ROWS] = {13, 12, 14, 27};
byte colPins[KEYPAD_COLS] = {26, 25, 33, 32};
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// Create Integrall system
Integrall::System integrall;

// OLED Display (128x64, I2C address 0x3C)
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Keypad (using Keypad library - add to platformio.ini)
// For now, we'll implement simple keypad reading
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

// Security system state
String enteredCode = "";
const String SECRET_CODE = "1234";
bool systemLocked = true;
unsigned long lastTelemetry = 0;
unsigned long lastDisplayUpdate = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║     Integrall FullDemo v0.1        ║");
    Serial.println("║  Multi-Module IoT Demonstration    ║");
    Serial.println("╚════════════════════════════════════╝\n");
    
    // Initialize OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
    } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println(F("Integrall Boot..."));
        display.display();
    }
    
    // Configure Integrall
    Integrall::DeviceConfig config;
    config.wifi_ssid = WIFI_SSID;
    config.wifi_password = WIFI_PASSWORD;
    config.backend_url = BACKEND_URL;
    config.api_key = API_KEY;
    config.poll_interval_ms = 2000;  // Poll every 2 seconds for responsiveness
    
    // Initialize Integrall
    if (!integrall.begin(config)) {
        Serial.println("❌ Failed to initialize Integrall!");
        showError("Init Failed");
        return;
    }
    
    // Setup Relays with safety features
    Serial.println("\n🔌 Configuring Relays...");
    
    int relay1 = integrall.enableRelay(RELAY_1_PIN, true, "Light");
    integrall.relaySetTimeout(relay1, 300000);  // 5 minute safety timeout
    
    int relay2 = integrall.enableRelay(RELAY_2_PIN, true, "Fan");
    integrall.relaySetTimeout(relay2, 600000);  // 10 minute safety timeout
    integrall.relaySetInterlock(relay2, 1);     // Interlock group 1
    
    int relay3 = integrall.enableRelay(RELAY_3_PIN, true, "Heater");
    integrall.relaySetTimeout(relay3, 1800000); // 30 minute safety timeout
    integrall.relaySetInterlock(relay3, 1);     // Same interlock as Fan (can't run together)
    
    int relay4 = integrall.enableRelay(RELAY_4_PIN, true, "Pump");
    integrall.relaySetTimeout(relay4, 120000);  // 2 minute safety timeout
    
    Serial.println("✅ Relays configured with safety interlocks");
    
    // Setup Keypad
    Serial.println("\n⌨️  Configuring Keypad...");
    keypad.setDebounceTime(50);
    keypad.setHoldTime(1000);
    Serial.println("✅ Keypad ready (Code: " + SECRET_CODE + ")");
    
    // Initial display
    updateDisplay();
    
    Serial.println("\n✨ System Ready!");
    Serial.println("Device ID: " + String(integrall.getDeviceId()));
    Serial.println("IP: " + String(integrall.getIPAddress()));
    Serial.println("\nCommands:");
    Serial.println("  - Enter code + # to unlock");
    Serial.println("  - A = Relay 1 toggle");
    Serial.println("  - B = Relay 2 toggle");
    Serial.println("  - C = All off");
    Serial.println("  - D = Status");
    Serial.println("  - * = Clear input");
}

void loop() {
    // Handle Integrall (WiFi, backend, modules)
    integrall.handle();
    
    // Handle keypad input
    handleKeypad();
    
    // Update display periodically
    if (millis() - lastDisplayUpdate > 500) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
    
    // Send telemetry every 30 seconds
    if (millis() - lastTelemetry > 30000) {
        sendTelemetry();
        lastTelemetry = millis();
    }
    
    // Handle backend commands
    handleBackendCommands();
    
    delay(10);  // Small delay for stability
}

void handleKeypad() {
    char key = keypad.getKey();
    
    if (key) {
        Serial.print("Key pressed: ");
        Serial.println(key);
        
        if (key == '*') {
            // Clear input
            enteredCode = "";
            Serial.println("Input cleared");
        }
        else if (key == '#') {
            // Check code
            if (enteredCode == SECRET_CODE) {
                systemLocked = !systemLocked;
                Serial.println(systemLocked ? "🔒 System LOCKED" : "🔓 System UNLOCKED");
                
                if (systemLocked) {
                    integrall.allRelaysOff();
                }
                
                // Flash display
                flashDisplay(systemLocked ? "LOCKED" : "UNLOCKED");
            } else {
                Serial.println("❌ Invalid code!");
                flashDisplay("INVALID");
            }
            enteredCode = "";
        }
        else if (key >= '0' && key <= '9') {
            // Add digit to code
            if (enteredCode.length() < 8) {
                enteredCode += key;
            }
        }
        else if (!systemLocked) {
            // Command keys (only work when unlocked)
            switch(key) {
                case 'A':
                    integrall.relayToggle(0);
                    Serial.println("Toggle Relay 1 (Light)");
                    break;
                case 'B':
                    integrall.relayToggle(1);
                    Serial.println("Toggle Relay 2 (Fan)");
                    break;
                case 'C':
                    integrall.allRelaysOff();
                    Serial.println("All relays OFF");
                    break;
                case 'D':
                    printStatus();
                    break;
            }
        }
    }
}

void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Header
    display.setCursor(0,0);
    display.print(F("Integrall "));
    display.print(systemLocked ? F("[LOCKED]") : F("[ACTIVE]"));
    
    // Connection status
    display.setCursor(0,10);
    if (integrall.isOnline()) {
        display.print(F("🟢 Online"));
    } else if (integrall.isWiFiConnected()) {
        display.print(F("🟡 WiFi OK"));
    } else {
        display.print(F("🔴 Offline"));
    }
    
    // Signal strength
    int rssi = integrall.getWiFiStrength();
    display.print(F(" "));
    display.print(rssi);
    display.print(F("dBm"));
    
    // Relay states
    display.setCursor(0,22);
    display.print(F("Relays: "));
    display.print(integrall.relayIsOn(0) ? F("1:ON ") : F("1:off "));
    display.print(integrall.relayIsOn(1) ? F("2:ON ") : F("2:off "));
    
    display.setCursor(0,32);
    display.print(F("        "));
    display.print(integrall.relayIsOn(2) ? F("3:ON ") : F("3:off "));
    display.print(integrall.relayIsOn(3) ? F("4:ON") : F("4:off"));
    
    // Code entry
    display.setCursor(0,44);
    display.print(F("Code: "));
    for (int i = 0; i < enteredCode.length(); i++) {
        display.print(F("*"));
    }
    
    // Footer
    display.setCursor(0,56);
    display.print(F("A:1 B:2 C:AllOff D:Stat"));
    
    display.display();
}

void flashDisplay(const char* message) {
    for (int i = 0; i < 3; i++) {
        display.invertDisplay(true);
        delay(100);
        display.invertDisplay(false);
        delay(100);
    }
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 24);
    display.print(message);
    display.display();
    delay(1000);
}

void showError(const char* error) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print(F("ERROR:"));
    display.setCursor(0,16);
    display.print(error);
    display.display();
}

void printStatus() {
    Serial.println("\n=== System Status ===");
    Serial.println("Lock: " + String(systemLocked ? "LOCKED" : "UNLOCKED"));
    Serial.println("WiFi: " + String(integrall.isWiFiConnected() ? "Connected" : "Disconnected"));
    Serial.println("Backend: " + String(integrall.isOnline() ? "Online" : "Offline"));
    Serial.println("RSSI: " + String(integrall.getWiFiStrength()) + " dBm");
    Serial.println("Relays: " + 
        String(integrall.relayIsOn(0) ? "1:ON " : "1:off ") +
        String(integrall.relayIsOn(1) ? "2:ON " : "2:off ") +
        String(integrall.relayIsOn(2) ? "3:ON " : "3:off ") +
        String(integrall.relayIsOn(3) ? "4:ON" : "4:off")
    );
    Serial.println("===================\n");
}

void sendTelemetry() {
    StaticJsonDocument<512> doc;
    
    doc["system_locked"] = systemLocked;
    doc["relay_states"] = JsonArray();
    doc["relay_states"].add(integrall.relayIsOn(0));
    doc["relay_states"].add(integrall.relayIsOn(1));
    doc["relay_states"].add(integrall.relayIsOn(2));
    doc["relay_states"].add(integrall.relayIsOn(3));
    doc["free_heap"] = ESP.getFreeHeap();
    doc["wifi_rssi"] = integrall.getWiFiStrength();
    
    if (integrall.sendTelemetry(doc)) {
        Serial.println("📊 Telemetry sent");
    } else {
        Serial.println("⚠️  Telemetry failed");
    }
}

void handleBackendCommands() {
    // This is handled automatically by integrall.handle()
    // But we can add custom command processing here if needed
    
    // Example: Check if any relay states changed via backend
    // and update display accordingly
}
