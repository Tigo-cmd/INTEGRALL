/**
 * KeypadLock.ino - Password Door Lock System
 * 
 * The "Integrall" way: full PIN lock system in 20 lines.
 * 
 * Integrall automatically handles:
 *   ✅ Key scanning & debouncing
 *   ✅ Asterisk (*) display on LCD as you type
 *   ✅ Backspace (*) and Submit (#) keys
 *   ✅ "Access Denied" / "Unlocked" LCD messages
 *   ✅ Remaining attempts counter
 *   ✅ Lockout after too many wrong attempts
 *   ✅ Auto-lock relay after unlock duration
 * 
 * Hardware:
 *   Keypad ROWS  -> GPIO 19, 18, 5, 17
 *   Keypad COLS  -> GPIO 16, 4, 2, 15
 *   LCD SDA/SCL  -> GPIO 21, 22
 *   Relay        -> GPIO 13
 */

#define INTEGRALL_ENABLE_KEYPAD
#define INTEGRALL_ENABLE_LCD
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>

Integrall::System integrall;

byte rowPins[4] = {19, 18, 5, 17};
byte colPins[4] = {16,  4, 2, 15};

void setup() {
    integrall.begin();

    int door = integrall.enableRelay(13, true, "Door");
    integrall.enableKeypad(rowPins, colPins);

    // One line sets up everything:
    // PIN, relay index, unlock duration, max wrong attempts
    integrall.lockSetup("1234", door, 5000, 3);
}

void loop() {
    integrall.lockUpdate(); // Runs the entire lock system
    integrall.handle();     // Keeps WiFi & background tasks alive
}
