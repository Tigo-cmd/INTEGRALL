/**
 * BasicRelay.ino — one relay, one push-button, a safety timeout. Works offline
 * on any board (ESP32, ESP8266, Uno/Nano).
 *
 *  - Relay module IN  -> pin 5  (active-low board, the common blue modules)
 *  - Push button      -> pin 4 to GND (internal pull-up is used)
 *  - Serial: send '1' = ON, '0' = OFF, 't' = toggle, 's' = status
 *
 * The relay turns itself off after 30 s (safety timeout) no matter what.
 * For cloud control see the CloudMQTT example.
 */
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>

Integrall::System integrall;
int relay = -1;

void setup() {
    integrall.begin();                                // offline mode, starts Serial at 115200

    relay = integrall.enableRelay(5, true, "Main Relay");
    if (relay < 0) {
        integrall.println("Relay could not be enabled");
        return;
    }
    integrall.relaySetTimeout(relay, 30000);          // auto-off after 30 s
    integrall.relayAttachButton(relay, 4);            // physical toggle button
    integrall.println("Relay ready. Send 1 / 0 / t / s");
}

void loop() {
    integrall.handle();                               // timeouts + debounced button, never blocks

    if (Serial.available()) {
        switch (Serial.read()) {
            case '1': integrall.relayOn(relay); break;
            case '0': integrall.relayOff(relay); break;
            case 't': integrall.relayToggle(relay); break;
            case 's':
                integrall.println("Relay is %s, on for %lu ms total, toggled %lu times",
                                  integrall.relayIsOn(relay) ? "ON" : "OFF",
                                  (unsigned long)integrall.relayGetOnTime(relay),
                                  (unsigned long)integrall.relayGetToggleCount(relay));
                break;
        }
    }
}
