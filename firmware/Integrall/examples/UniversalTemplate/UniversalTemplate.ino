/**
 * UniversalTemplate.ino - How to use Integrall with ANY custom code
 * 
 * This example shows how Integrall acts as a foundation (OS) for your project.
 * It handles the "boring" parts (WiFi, Cloud, Monitoring, Timing) while you focus
 * on your custom logic (Bluetooth, AI, special sensors, etc.).
 */

// 1. Enable ONLY what you need to keep it light
#define INTEGRALL_ENABLE_LCD
#define INTEGRALL_ENABLE_BUZZER
#include <Integrall.h>

// Imagine you are using a library Integrall doesn't know about yet
// For example: Bluetooth or a specialized Co2 Sensor
// #include <BluetoothSerial.h> 

Integrall::System integrall;

void setup() {
    // 2. Integrall handles all the "standard" IoT setup in one line
    integrall.begin();

    // 3. Setup your custom components here
    Serial.println("Custom Sensor initializing...");
    // SerialBT.begin("MyIntegrallDevice");
}

void loop() {
    // 4. CRITICAL: integrall.handle() keeps the system alive in the background.
    // It processes network commands and non-blocking timers while your loop runs.
    integrall.handle();

    // 5. USE INTEGRALL HELPERS TO SIMPLIFY YOUR CUSTOM LOGIC
    
    // Example: Instead of writing a complex timer for a heartbeat
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        lastCheck = millis();
        
        // Use integrall to report ANY data to the cloud in one line
        StaticJsonDocument<128> doc;
        doc["my_custom_data"] = analogRead(A0); // Even if it's not an "Integrall" sensor
        integrall.sendTelemetry(doc);
        
        // Use integrall for instant UI feedback
        integrall.lcdPrint("Data Sent!", 0, 1);
        integrall.buzzerBeep(50); // Short confirmation chirp
    }

    // Example: Handling custom input (like Bluetooth or Serial)
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'A') {
            // Trigger an Integrall "Project" behavior manually if you want
            // Or just use the modules directly
            integrall.blink(2, 100); // Visual "Alert" on pin 2
        }
    }
}
