/**
 * ButtonBuzzer — press a button to turn a buzzer on, press again to turn it off.
 * The same buzzer is a switch in the cloud (OpenRemote): flip it there and the
 * device follows; press the button and the dashboard follows.
 *
 * Hardware (ESP32 DevKit)
 *   Push button  : one leg -> GPIO 14, other leg -> GND   (internal pull-up, no resistor)
 *   Buzzer       : + -> GPIO 25, - -> GND
 *                  active buzzer module (sounds on plain 3.3 V): ACTIVE_BUZZER = true
 *                  passive buzzer / bare piezo:                  ACTIVE_BUZZER = false
 *   On-board LED : GPIO 2 mirrors the buzzer (handy if you have no buzzer yet)
 *
 * Works without WiFi too: leave WIFI_SSID empty and the button still toggles the buzzer.
 *
 * Cloud (optional)
 *   1. Register the device:  python cloud/tools/integrall_devices.py add <DEVICE ID> "Button buzzer"
 *      (the Device ID is printed on the Serial Monitor at boot)
 *   2. Paste the printed token into DEVICE_TOKEN, and your broker's address into BROKER_URL.
 *   3. In OpenRemote the asset "Button buzzer" gets a `buzzer` on/off switch,
 *      `presses` (count, with history) and `lastEvent` ("button: on"/"button: off").
 *
 * Libraries: ArduinoJson (v7), espMqttClient.
 */
#define INTEGRALL_ENABLE_MQTT           // implies INTEGRALL_ENABLE_WIFI
#define INTEGRALL_ENABLE_BUZZER
#define INTEGRALL_ENABLE_INPUT
#include <Integrall.h>

// ---- settings: put them in secrets.h (kept out of git), or edit the defaults ----
#if __has_include("secrets.h")
  #include "secrets.h"
#endif
#ifndef WIFI_SSID
  #define WIFI_SSID    "YourWiFi"                     // "" = offline mode
  #define WIFI_PASS    "YourPassword"
#endif
#ifndef BROKER_URL
  #define BROKER_URL   "mqtt://192.168.1.10:1883"     // your PC's LAN IP
#endif
#ifndef DEVICE_TOKEN
  #define DEVICE_TOKEN "token-from-integrall_devices.py"
#endif
#ifndef ACTIVE_BUZZER
  #define ACTIVE_BUZZER true                          // false for a passive buzzer / piezo
#endif
// -------------------------------------------------------------------------------

const uint8_t BUTTON_PIN = 14;
const uint8_t BUZZER_PIN = 25;
const uint8_t LED_PIN    = 2;

Integrall::System integrall;
unsigned long presses = 0;

void setBuzzer(bool on) {
    if (on) integrall.buzzerOn(); else integrall.buzzerOff();
    digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void setup() {
    Integrall::DeviceConfig cfg;
    cfg.wifi_ssid     = WIFI_SSID;
    cfg.wifi_password = WIFI_PASS;
    cfg.backend_url   = BROKER_URL;
    cfg.api_key       = DEVICE_TOKEN;
    integrall.begin(cfg);                       // returns immediately; WiFi/MQTT connect in background

    pinMode(LED_PIN, OUTPUT);
    integrall.enableBuzzer(BUZZER_PIN, ACTIVE_BUZZER);

    // The buzzer as a cloud switch: OpenRemote shows it as an on/off attribute named "buzzer".
    integrall.addCloudSwitch("buzzer",
        [] { return integrall.buzzerIsOn(); },
        [](bool on) { setBuzzer(on); });

    integrall.println("Device ID: %s", integrall.getDeviceId());
    integrall.println("Press the button on GPIO %d to toggle the buzzer", BUTTON_PIN);
}

void loop() {
    integrall.handle();                         // WiFi, MQTT, buzzer... never blocks

    if (integrall.inputButtonPressed(BUTTON_PIN)) {          // debounced, fires once per press
        const bool on = !integrall.buzzerIsOn();
        setBuzzer(on);
        presses++;
        integrall.println("Button -> buzzer %s", on ? "ON" : "OFF");
        integrall.sendEvent("button", on ? "on" : "off");      // no-op while offline
        integrall.sendTelemetry("presses", (float)presses);
    }
}
