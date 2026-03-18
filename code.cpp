// ❌ THE OLD WAY (Boilerplate Nightmare)
#include <WiFi.h>
const char* ssid = "MyWiFi";
const char* password = "MyPassword";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

// ==========================================

// ✅ THE INTEGRALL WAY (Clean Logic)
#define INTEGRALL_ENABLE_WIFI
#include <Integrall.h>

Integrall::System integrall;

void setup() {
  // Handles connection, IP assignment, and background reconnects
  integrall.begin("MyWiFi", "MyPassword"); 
}
