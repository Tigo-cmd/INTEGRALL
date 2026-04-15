/**
 * SmartFallDevice.ino
 * 
 * Powered by Integrall Framework.
 * A lightweight, non-blocking Fall Detection logic using MPU6050 and SIM800L.
 * Creates its own WiFi hotspot to host a Web Dashboard.
 */

#define INTEGRALL_ENABLE_WIFI
#define INTEGRALL_ENABLE_CELLULAR
#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_IMU
#define INTEGRALL_ENABLE_BUZZER
#define INTEGRALL_ENABLE_INPUT
#define INTEGRALL_DEBUG_LEVEL 3
#include <Integrall.h>

// Global Integrall Instance
Integrall::System integrall;

// Include the separate dashboard logic
#include "Dashboard.h"

// Alert states
bool alarmActive = false;
unsigned long fallTime = 0;

void setup() {
    // 1. Integrall Initialization
    // Creates an open Hotspot (AP Mode) so you can connect directly with your phone!
    integrall.beginAP("Integrall_Fall_Device");

    // Start Integrall Internal Modules
    integrall.enableIMU(21, 22);                    // I2C MPU6050 Data/Clock
    integrall.enableBuzzer(27);                     // Buzzer Pin
    
    // Manual Button Pin (InputModule is for debounced clicks, we need raw holds)
    pinMode(26, INPUT_PULLUP);
    
    // Uncomment once your SIM800L module is wired!
    // integrall.cellularBegin(16, 17);  
    
    // 2. Start Dashboard
    setupDashboard();
}

void loop() {
    integrall.handle();        // Background network recovery & AT commands
    handleDashboard();         // Handle local Web Dashboard

    // ==========================================
    // ADVANCED FALL DETECTION LOGIC
    // ==========================================
    if (!alarmActive) {
        // Did a sudden 20G impact occur AND did device instantly become horizontal?
        if (integrall.getImpactForce() > 20.0 && integrall.isHorizontal()) {
            
            // Wait 2 seconds to see if they immediately stand back up
            delay(2000); 
            
            // False Alarm Check! Are they still horizontal? (i.e. lying on the floor)
            if (integrall.isHorizontal()) {
                INTEGRALL_LOG_INFO("⚠ CRITICAL FALL VERIFIED!");
                alarmActive = true;
                fallTime = millis();
                
                // Integrall takes over the continuous buzzing pattern here
                integrall.buzzerAlert(); 
            }
        }
    }

    // ==========================================
    // SMS DISPATCH TIMER
    // ==========================================
    if (alarmActive && (millis() - fallTime > 10000)) {
        INTEGRALL_LOG_INFO(">> Timeout reached! Sending automated SMS...");
        integrall.buzzerOff();
        
        // emergencyNumber is pulled from the Dashboard (EEPROM)
        integrall.sendSMS(emergencyNumber.c_str(), "ALERT! Fall Detected. Please check on the user.");
        
        alarmActive = false; 
        delay(5000); // Prevent rapid re-triggering
    }

    // ==========================================
    // MANUAL CANCEL BUTTON (3 Second Hold)
    // ==========================================
    if (alarmActive && digitalRead(26) == LOW) {
        unsigned long holdStart = millis();
        bool cancelled = true;
        
        while (millis() - holdStart < 3000) {
            // If they let go of the button early, abort cancel
            if (digitalRead(26) == HIGH) {
                cancelled = false; 
                break;
            }
            delay(10); 
        }
        
        if (cancelled) {
            INTEGRALL_LOG_INFO(">> Alarm Manually Cancelled!");
            alarmActive = false;
            integrall.buzzerOff();
        }
    }
}
