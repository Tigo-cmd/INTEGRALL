/**
 * FullDemo.ino - The "Integrall" Way
 * 
 * This demo shows how to use Relays and the LCD with almost ZERO boilerplate.
 * The framework handles:
 * - WiFi connection & Backend sync
 * - I2C initialization
 * - LCD status updates
 * - Relay safety logic
 */

#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_LCD
#define INTEGRALL_DEBUG_LEVEL 3
#include <Integrall.h>

Integrall::System integrall;

void setup() {
    // 1. Basic configuration (WiFi & Backend)
    Integrall::DeviceConfig config;
    config.wifi_ssid     = "YOUR_SSID";
    config.wifi_password = "YOUR_PASSWORD";
    config.backend_url   = "http://192.168.1.100:8000";
    config.api_key       = "your-secret-key";
    
    // 2. Start the system - Handles all hardware init automatically
    integrall.begin(config);
    
    // 3. Simple Modules - Setup in one line
    integrall.enableRelay(5, true, "Light");
    
    // 4. Easy Display - No Wire.h or address management needed
    integrall.lcdPrint("Integrall v0.1", 0, 0);
    integrall.lcdPrint("System Ready", 0, 1);
}

void loop() {
    // 5. Just call handle() - Framework does everything else!
    integrall.handle();
}
