/**
 * WebDashboardAndHTTPS.ino
 *
 * Demonstrates Integrall's full Web & Networking stack:
 * 1. HTTPS & HTTP REST Client (GET, POST, PUT, PATCH, DELETE, Bearer Auth, Custom Headers)
 * 2. C++ HTML Tag & Responsive IoT Dashboard Renderer (HtmlBuilder)
 * 3. Local HTTP/REST Web Server (onGet, onPost, onPut, onDelete, servePage, webSendJson)
 * 4. Native Zero-Dependency RFC 6455 WebSockets (wsServerBegin, wsBroadcast, onWsMessage)
 */

#define INTEGRALL_ENABLE_WIFI
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_DEBUG_LEVEL 3
#include <Integrall.h>

Integrall::System integrall;
int relayIdx = -1;
int targetTemp = 24;
unsigned long lastWsPush = 0;

void buildAndServeDashboard() {
    // Render a complete responsive dark-mode IoT dashboard using C++ HTML tag helpers
    String metricsRow = integrall.htmlTag("div",
        integrall.htmlMetric("Temperature", "25.4", " °C", "temp") +
        integrall.htmlMetric("Humidity", "58", " %", "hum") +
        integrall.htmlMetric("WiFi Signal", String(integrall.getWiFiStrength()), " dBm", "rssi"),
        "class=\"ig-grid\""
    );

    String controlsCard = integrall.htmlCard("Device Controls",
        integrall.htmlToggle("Main Power Relay", "pwr", false, "/api/relay") +
        integrall.htmlSlider("Target Setpoint (°C)", "setpoint", 16, 32, targetTemp, "/api/setpoint") +
        integrall.htmlButton("Trigger Cloud Sync", "/api/sync", "cyan") +
        integrall.htmlButton("Emergency Cutoff", "/api/relay?state=0", "red")
    );

    String wsStatusCard = integrall.htmlCard("Live WebSocket Stream (Port 81)",
        integrall.htmlTag("p", "Waiting for real-time telemetry...", "id=\"ws-log\"")
    );

    String fullHtml = integrall.htmlPage(
        "Integrall ESP32 Web Hub",
        integrall.htmlTag("h1", "Integrall Live Control Center ") +
        integrall.htmlBadge("ONLINE", "green") +
        metricsRow +
        controlsCard +
        wsStatusCard +
        integrall.htmlWsScript(81, "ws-log")
    );

    integrall.servePage("/", fullHtml);
}

void setup() {
    integrall.begin("YourWiFiSSID", "YourWiFiPassword");
    relayIdx = integrall.enableRelay(26, true, "MainPower");

    // ------------------------------------------------------------------------
    // 1. HTTPS REST Client Example (GET / POST / PUT / DELETE)
    // ------------------------------------------------------------------------
    if (integrall.isWiFiConnected()) {
        integrall.setBearerToken("my-secret-jwt-token");
        integrall.setHttpHeader("X-Device-Name", "Integrall-Node-01");

        // Automatic TLS/SSL when URL starts with https://
        String getResp = integrall.httpGet("https://httpbin.org/get");
        int postCode   = integrall.httpPost("https://httpbin.org/post", "{\"status\":\"booted\"}");
        int putCode    = integrall.httpPut("https://httpbin.org/put", "{\"targetTemp\":24}");
        int delCode    = integrall.httpDelete("https://httpbin.org/delete");
        integrall.println("HTTPS Codes -> POST:%d PUT:%d DELETE:%d (GET len:%d)",
                          postCode, putCode, delCode, getResp.length());
    }

    // ------------------------------------------------------------------------
    // 2. Local HTTP/REST Web Server & HTML Dashboard (Port 80)
    // ------------------------------------------------------------------------
    integrall.webServerBegin(80);
    buildAndServeDashboard();

    integrall.onGet("/api/relay", []() {
        int state = integrall.webArg("state").toInt();
        integrall.setRelay(relayIdx, state == 1);
        integrall.webSendJson(String("{\"relay\":") + (state == 1 ? "true" : "false") + "}");
    });

    integrall.onGet("/api/setpoint", []() {
        targetTemp = integrall.webArg("val").toInt();
        integrall.webSendJson(String("{\"setpoint\":") + String(targetTemp) + "}");
    });

    integrall.onPost("/api/control", []() {
        String rawBody = integrall.webBody();
        integrall.println("Received POST JSON: %s", rawBody.c_str());
        integrall.webSendJson("{\"ok\":true}");
    });

    // ------------------------------------------------------------------------
    // 3. Real-Time RFC 6455 WebSocket Server (Port 81)
    // ------------------------------------------------------------------------
    integrall.wsServerBegin(81);
    integrall.onWsMessage([](const char* msg) {
        integrall.println("WebSocket Message Received: %s", msg);
        if (strcmp(msg, "ping") == 0) {
            integrall.wsBroadcast("{\"ws-log\":\"pong from ESP32!\"}");
        }
    });

    integrall.println("Dashboard ready at http://%s/", integrall.getIPAddress());
}

void loop() {
    // Non-blocking pump handles WiFi, WebServer, and WebSockets automatically
    integrall.handle();

    // Push live JSON updates to all connected WebSocket browsers every 2 seconds
    if (millis() - lastWsPush >= 2000) {
        lastWsPush = millis();
        if (integrall.wsClientCount() > 0) {
            String liveJson = String("{\"temp\":\"") + String(24.0 + (millis() % 30) * 0.1, 1) +
                              "\",\"hum\":\"" + String(55 + (millis() % 10)) +
                              "\",\"rssi\":\"" + String(integrall.getWiFiStrength()) + "\"}";
            integrall.wsBroadcast(liveJson.c_str());
        }
    }
}
