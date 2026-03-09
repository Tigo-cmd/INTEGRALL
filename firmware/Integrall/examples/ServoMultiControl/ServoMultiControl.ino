/**
 * ServoMultiControl.ino
 * 
 * Control multiple Servo motors simultaneously without writing complex code!
 * 
 * Integrall allows you to setup multiple servos by capturing their "ID". 
 * You can then control them totally independently in the background.
 */

#define INTEGRALL_ENABLE_SERVO
#include <Integrall.h>

Integrall::System integrall;

// Global variables to store the Servo IDs
int mainArm;
int robotClaw;
int radarScanner;

void setup() {
    integrall.begin();
    
    // 1. Initialize servos and save their IDs. 
    // We optionally set their starting angles too!
    
    // Attach mainArm to pin 10, start at 90 degrees
    mainArm = integrall.enableServo(10, 90); 
    
    // Attach robotClaw to pin 11, start closed at 0 degrees
    robotClaw = integrall.enableServo(11, 0);
    
    // Attach radarScanner to pin 12, start at 90 degrees
    radarScanner = integrall.enableServo(12, 90);

    integrall.println("Multi-Servo System Ready!");
    
    // 2. Let's make the radarScanner constantly sweep back and forth in the background!
    // 15 = speed (delay in milliseconds between each degree)
    integrall.sweepServo(radarScanner, 15);
}

void loop() {
    static unsigned long last_action = 0;
    
    // Every 5 seconds, let's trigger an action for the Arm and Claw
    if (millis() - last_action > 5000) {
        
        integrall.println("Opening claw and lowering arm!");
        
        // Use the ID to control the specific motor
        integrall.easeServo(robotClaw, 180, 5); // Open claw fast (5ms per degree)
        integrall.easeServo(mainArm, 0, 30);    // Lower arm slowly (30ms per degree)
        
        last_action = millis();
    }
    
    // This perfectly updates the background animations for ALL 3 motors simultaneously!
    integrall.handle();
}
