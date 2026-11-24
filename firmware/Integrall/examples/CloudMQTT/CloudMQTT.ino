/**
 * CloudMQTT — Integrall device connected to the Integrall cloud over MQTT.
 *
 * What it shows
 *  - Non-blocking WiFi + MQTT: loop() keeps running while (re)connecting.
 *  - Telemetry every 10 s (DHT22 temperature/humidity + relay state).
 *  - Cloud commands pushed instantly (no polling): the built-in
 *    "relay.set" / "relay.toggle" / "ping" / "reboot" / "health" work
 *    out of the box; "led.blink" below shows a custom command.
 *  - Self-test: which parts are really connected, printed at boot and
 *    published as retained "health" for the dashboard.
 *
 * Hardware (all optional — missing parts are reported, not fatal)
 *  - Relay module IN -> GPIO 26 (active-low board)
 *  - DHT22 data      -> GPIO 4
 *
 * Libraries: ArduinoJson (v7), espMqttClient, DHT sensor library
 * (+ ESPAsyncTCP on ESP8266).
 *
 * Try it without the cloud backend using any MQTT broker, e.g.
 *   mosquitto_sub -h <broker> -t 'integrall/#' -v
 *   mosquitto_pub -h <broker> -t 'integrall/<DEVICE_ID>/cmd' \
 *       -m '{"id":"1","action":"relay.toggle","args":{"index":0}}'
 */
#define INTEGRALL_ENABLE_MQTT          // implies INTEGRALL_ENABLE_WIFI
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_SENSORS
#include <Integrall.h>

// ---- change these ------------------------------------------------------------
const char* WIFI_SSID    = "YourWiFi";
const char* WIFI_PASS    = "YourPassword";
const char* BROKER_URL   = "mqtt://192.168.1.10:1883";   // or "mqtts://host:8883"
const char* DEVICE_TOKEN = "token-from-integrall_devices.py";   // MQTT password
// -------------------------------------------------------------------------------

const uint8_t RELAY_PIN = 26;
const uint8_t DHT_PIN   = 4;
#if defined(ESP8266)
const uint8_t LED_PIN   = 2;    // NodeMCU on-board LED
#else
const uint8_t LED_PIN   = 2;    // ESP32 DevKit on-board LED
#endif

Integrall::System integrall;
int pump = -1;

// Custom cloud command. Leave `reply` untouched to fall back to the built-ins.
void onCloudCommand(const char* action, JsonVariantConst args, Integrall::CommandReply& reply) {
    if (strcmp(action, "led.blink") == 0) {
        uint32_t ms = args["interval"] | 300;
        integrall.blink(LED_PIN, ms);
        reply.ok("blinking");
    } else if (strcmp(action, "led.off") == 0) {
        integrall.stopBlink();
        reply.ok();
    }
}

void onWiFi(bool connected) {
    integrall.println(connected ? "WiFi up, IP %s" : "WiFi down%s", connected ? integrall.getIPAddress() : "");
}

void setup() {
    Integrall::DeviceConfig cfg;
    cfg.wifi_ssid     = WIFI_SSID;
    cfg.wifi_password = WIFI_PASS;
    cfg.backend_url   = BROKER_URL;
    cfg.api_key       = DEVICE_TOKEN;
    integrall.onConnectionChange(onWiFi);
    integrall.begin(cfg);                       // returns immediately

    pump = integrall.enableRelay(RELAY_PIN, true, "pump");
    integrall.onCommand(onCloudCommand);

    integrall.println("Device ID: %s", integrall.getDeviceId());
    integrall.selfTest();                       // prints which parts answer
}

unsigned long lastTelemetry = 0;

void loop() {
    integrall.handle();                         // WiFi, MQTT, relays, sensors... never blocks

    if (millis() - lastTelemetry >= 10000) {
        lastTelemetry = millis();
        JsonDocument t;
        float temp = integrall.readTemperature(DHT_PIN, 22);
        float hum  = integrall.readHumidity(DHT_PIN, 22);
        if (Integrall::valid(temp)) t["temperature"] = temp;   // skip missing sensor values
        if (Integrall::valid(hum))  t["humidity"] = hum;
        t["relay0"] = integrall.relayIsOn(pump);
        t["rssi"]   = integrall.getWiFiStrength();
        if (!integrall.sendTelemetry(t)) {
            integrall.println("telemetry not sent (cloud %s)", integrall.isCloudConnected() ? "up" : "down");
        }
    }
}
