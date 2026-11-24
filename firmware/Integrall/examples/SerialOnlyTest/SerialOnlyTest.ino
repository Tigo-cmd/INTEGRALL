/**
 * SerialOnlyTest.ino — run Integrall with NO hardware attached.
 *
 * Prints the device identity, uptime and a self-test (I2C scan + which
 * enabled parts answer) to the Serial Monitor every 10 s. Handy as the very
 * first sketch on a new board: if this runs, the toolchain and board are OK.
 */
#include <Integrall.h>

Integrall::System integrall;

void setup() {
    integrall.begin();                     // offline; opens Serial at 115200
    integrall.println("Device ID: %s", integrall.getDeviceId());
}

void loop() {
    integrall.handle();

    static unsigned long last = 0;
    if (millis() - last >= 10000) {
        last = millis();
        integrall.println("[report] uptime %lu s, state %s", millis() / 1000UL, integrall.getStatusString());
        #if defined(ESP32) || defined(ESP8266)
        integrall.println("[report] free heap %lu bytes", (unsigned long)ESP.getFreeHeap());
        #endif
        integrall.selfTest();              // I2C scan + module presence table
    }
}
