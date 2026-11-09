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

Integrall::System sys;
int relayIdx = -1;
int targetTemp = 24;
unsigned long lastWsPush = 0;

void buildAndServeDashboard() {
    // Render a complete responsive dark-mode IoT dashboard using C++ HTML tag helpers
    String metricsRow = sys.htmlTag("div",
        sys.htmlMetric("Temperature", "25.4", " °C", "temp") +
        sys.htmlMetric("Humidity", "58", " %", "hum") +
        sys.htmlMetric("WiFi Signal", String(sys.getWiFiStrength()), " dBm", "rssi"),
        "class=\"ig-grid\""
    );

    String controlsCard = sys.htmlCard("Device Controls",
        sys.htmlToggle("Main Power Relay", "pwr", false, "/api/relay") +
        sys.htmlSlider("Target Setpoint (°C)", "setpoint", 16, 32, targetTemp, "/api/setpoint") +
        sys.htmlButton("Trigger Cloud Sync", "/api/sync", "cyan") +
        sys.htmlButton("Emergency Cutoff", "/api/relay?state=0", "red")
    );

    String wsStatusCard = sys.htmlCard("Live WebSocket Stream (Port 81)",
        sys.htmlTag("p", "Waiting for real-time telemetry...", "id=\"ws-log\"")
    );

    String fullHtml = sys.htmlPage(
        "Integrall ESP32 Web Hub",
        sys.htmlTag("h1", "Integrall Live Control Center ") +
        sys.htmlBadge("ONLINE", "green") +
        metricsRow +
        controlsCard +
        wsStatusCard +
        sys.htmlWsScript(81, "ws-log")
    );

    sys.servePage("/", fullHtml);
}

void setup() {
    sys.begin("YourWiFiSSID", "YourWiFiPassword");
    relayIdx = sys.enableRelay(26, true, "MainPower");

    // ------------------------------------------------------------------------
    // 1. HTTPS REST Client Example (GET / POST / PUT / DELETE)
    // ------------------------------------------------------------------------
    if (sys.isWiFiConnected()) {
        sys.setBearerToken("my-secret-jwt-token");
        sys.setHttpHeader("X-Device-Name", "Integrall-Node-01");

        // Automatic TLS/SSL when URL starts with https://
        String getResp = sys.httpGet("https://httpbin.org/get");
        int postCode   = sys.httpPost("https://httpbin.org/post", "{\"status\":\"booted\"}");
        int putCode    = sys.httpPut("https://httpbin.org/put", "{\"targetTemp\":24}");
        int delCode    = sys.httpDelete("https://httpbin.org/delete");
        sys.println("HTTPS Codes -> POST:%d PUT:%d DELETE:%d (GET len:%d)",
                    postCode, putCode, delCode, getResp.length());
    }

    // ------------------------------------------------------------------------
    // 2. Local HTTP/REST Web Server & HTML Dashboard (Port 80)
    // ------------------------------------------------------------------------
    sys.webServerBegin(80);
    buildAndServeDashboard();

    sys.onGet("/api/relay", []() {
        int state = sys.webArg("state").toInt();
        sys.setRelay(relayIdx, state == 1);
        sys.webSendJson(String("{\"relay\":") + (state == 1 ? "true" : "false") + "}");
    });

    sys.onGet("/api/setpoint", []() {
        targetTemp = sys.webArg("val").toInt();
        sys.webSendJson(String("{\"setpoint\":") + String(targetTemp) + "}");
    });

    sys.onPost("/api/control", []() {
        String rawBody = sys.webBody();
        sys.println("Received POST JSON: %s", rawBody.c_str());
        sys.webSendJson("{\"ok\":true}");
    });

    // ------------------------------------------------------------------------
    // 3. Real-Time RFC 6455 WebSocket Server (Port 81)
    // ------------------------------------------------------------------------
    sys.wsServerBegin(81);
    sys.onWsMessage([](const char* msg) {
        sys.println("WebSocket Message Received: %s", msg);
        if (strcmp(msg, "ping") == 0) {
            sys.wsBroadcast("{\"ws-log\":\"pong from ESP32!\"}");
        }
    });

    sys.println("Dashboard ready at http://%s/", sys.getIPAddress());
}

void loop() {
    // Non-blocking pump handles WiFi, WebServer, and WebSockets automatically
    sys.handle();

    // Push live JSON updates to all connected WebSocket browsers every 2 seconds
    if (millis() - lastWsPush >= 2000) {
        lastWsPush = millis();
        if (sys.wsClientCount() > 0) {
            String liveJson = String("{\"temp\":\"") + String(24.0 + (millis() % 30) * 0.1, 1) +
                              "\",\"hum\":\"" + String(55 + (millis() % 10)) +
                              "\",\"rssi\":\"" + String(sys.getWiFiStrength()) + "\"}";
            sys.wsBroadcast(liveJson.c_str());
        }
    }
}
