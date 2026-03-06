/**
 * Tutorial: Serial to LCD & Buzzer
 * 
 * This example demonstrates the "Integrall Way" of communicating between 
 * different hardware components using simple one-liners.
 * 
 * Scenario:
 * 1. Open Serial Monitor (115200 baud).
 * 2. Type any message and press Enter.
 * 3. The framework will:
 *    - Display the message on your I2C LCD.
 *    - Play a "Success" melody on the buzzer.
 */

// 1. CHOOSE YOUR MODULES
// We only enable exactly what we need to keep the code fast and small.
#define INTEGRALL_ENABLE_LCD
#define INTEGRALL_ENABLE_BUZZER

#include <Integrall.h>

// 2. CREATE THE SYSTEM INSTANCE
Integrall::System integrall;

void setup() {
    // 3. START THE SYSTEM
    // begin() sets up Serial, internal timings, and enabled modules automatically.
    integrall.begin();
    
    // 4. INITIALIZE PINS
    // Connect your Buzzer to GPIO 4 (e.g., on ESP32)
    integrall.enableBuzzer(4);
    
    // I2C LCD starts automatically on standard pins (SDA/SCL)
    integrall.lcdPrint("Integrall Ready");
    integrall.lcdPrint("Type in Serial", 0, 1);
}

void loop() {
    // 5. READ SERIAL DATA
    if (Serial.available() > 0) {
        // Read the incoming string
        String incoming = Serial.readStringUntil('\n');
        incoming.trim(); // Remove whitespace
        
        if (incoming.length() > 0) {
            // 6. UPDATE LCD
            integrall.lcdClear();
            integrall.lcdPrint("Received:");
            integrall.lcdPrint(incoming.c_str(), 0, 1);
            
            // 7. FEEDBACK
            // Plays a professional success chime instantly
            integrall.buzzerSuccess();
            
            // Helpful logs
            Serial.print("Displayed on LCD: ");
            Serial.println(incoming);
        }
    }
    
    // 8. MANDATORY HANDLE CALL
    // Always call this to keep modules running smoothly (like buzzer patterns)
    integrall.handle();
}
