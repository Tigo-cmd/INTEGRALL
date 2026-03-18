// ❌ THE OLD WAY (Silent Failures & Crashes)
#include <WiFi.h>
#include <HTTPClient.h>

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "PASS");
}

void loop() {
  // Edge Case 1: WiFi drops unexpectedly. 
  // HTTPClient will block, timeout, and your code will freeze.
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://my-api.com/data");
    int code = http.GET(); // Freeze point
    http.end();
  } else {
    // You must manually trigger a WiFi.reconnect() here
    // But it blocks while reconnecting!
  }
}

// ==========================================

// ✅ THE INTEGRALL WAY (Resilient Error Handling)
#define INTEGRALL_DEBUG_LEVEL 4 // Set to VERBOSE logging
#define INTEGRALL_ENABLE_WIFI
#include <Integrall.h>

Integrall::System integrall;

void setup() {
  // 1. Integrall starts the Logger and begins async connection
  integrall.begin("MyWiFi", "MyPassword");
  
  // 2. We can log our own context using Flash-memory optimized macros
  INTEGRALL_LOG_INFO("Device booting. System checks initiated.");
}

void loop() {
  // The Background Engine handles WiFi reconnects silently and asynchronously!
  integrall.handle(); 

  // Edge Case Handling Example
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 10000) {
    lastUpdate = millis();

    // 3. Graceful checking: isOnline() verifies the chip is fully associated
    if (!integrall.isOnline()) {
      INTEGRALL_LOG_WARN("Network offline. Delaying API sync.");
      return; // Safely abort without crashing or blocking
    }
    
    // 4. Easy HTTP has built-in timeout handling and won't lock up memory
    String response = integrall.httpGet("http://api.myserver.com/sensors");
    
    if (response == "") {
      INTEGRALL_LOG_ERROR("HTTP GET Failed: Server timeout or unreachable.");
    } else {
      INTEGRALL_LOG_INFO_VAL("Data received, length: ", response.length());
    }
    
    // 5. Memory Monitoring
    // Prevent memory leaks by logging ESP32 heap fragmentation!
    Integrall::Logger::printMemoryStats(); 
  }
}
