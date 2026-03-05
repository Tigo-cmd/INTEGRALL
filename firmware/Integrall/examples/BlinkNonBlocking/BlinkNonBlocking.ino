/**
 * BlinkNonBlocking.ino - The "Integrall" Hello World
 * 
 * Traditional Arduino blink uses delay(1000), which freezes the CPU.
 * Integrall uses a non-blocking blinker, allowing the ESP32 to 
 * process WiFi, sensors, and commands while the LED blinks.
 * 
 * Hardware:
 *   Build-in LED -> GPIO 2 (on most ESP32 boards)
 */

#include <Integrall.h>

Integrall::System integrall;

void setup() {
    integrall.begin();

    // Start blinking GPIO 2 every 500ms
    // This happens in the background!
    integrall.blink(2, 500); 
}

void loop() {
    // No code needed here for the blinker!
    // You can add sensor reads or relay logic here
    // and they will run perfectly without being slowed down.

    integrall.handle(); // Keeps the blinker and network running
}
