/**
 * ServoKeypadControl.ino
 * 
 * Move a Servo motor to any angle using a Matrix Keypad!
 * Type the angle (e.g. "90" or "180") and press '#' to execute.
 * Press '*' to backspace if you make a mistake.
 */

#define INTEGRALL_ENABLE_KEYPAD
#define INTEGRALL_ENABLE_SERVO
#include <Integrall.h>

Integrall::System integrall;

// Adjust pins to match your wiring!
byte rowPins[4] = {9, 8, 7, 6};
byte colPins[4] = {5, 4, 3, 2};

void setup() {
    integrall.begin();
    
    // 1. Initialize Modules
    integrall.enableKeypad(rowPins, colPins);
    
    // Attach servo to Pin 10, and instantly set it to 0 degrees to start.
    integrall.enableServo(10, 0); 
    
    // Use the neat new one-liner print function
    integrall.println("System Ready!");
    integrall.println("Type an angle (0-180) and press '#' to move.");
}

void loop() {
    // Automatically capture keystrokes
    const char* input = integrall.keypadCapture(3); 
    
    char key = integrall.keypadGetKey();
    if (key != '\0' && key != '#' && key != '*') {
        // Print keystrokes beautifully on one line
        integrall.print("%c", key);
    }
    
    // Did they press '#'?
    if (integrall.keypadSubmitted()) {
        integrall.println(""); // New line
        
        // Convert the captured string to an integer, keep it 0-180
        int targetAngle = atoi(input); 
        if (targetAngle < 0) targetAngle = 0;
        if (targetAngle > 180) targetAngle = 180;
        
        integrall.println("Easing Servo to %d degrees...", targetAngle);
        
        // Use the new dynamic ease function to smoothly rotate it
        // 15 = 15ms delay between each degree
        integrall.easeServo(targetAngle, 15);
        
        integrall.keypadClear();
    }
    
    // Keep animations and everything running smoothly
    integrall.handle();
}
