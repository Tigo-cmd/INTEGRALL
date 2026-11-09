/**
 * DeviceManager.h
 * 
 * Core device management for Integrall Framework
 * Handles WiFi connection, HTTP/HTTPS REST client (GET, POST, PUT, PATCH, DELETE),
 * TLS/SSL security, backend registration, and device identity.
 */

#ifndef INTEGRALL_DEVICE_MANAGER_H
#define INTEGRALL_DEVICE_MANAGER_H

#include "../config/IntegrallConfig.h"
#include "Logger.h"

// Set to 0 to completely skip all backend logic (registration, telemetry, commands)
#ifndef INTEGRALL_BACKEND_ENABLED
  #define INTEGRALL_BACKEND_ENABLED 0
#endif

/**
 * ARCHITECTURE DETECTION & NETWORKING ENABLEMENT
 * Networking capability is compiled generically for ESP boards to avoid ODR violations
 * if sketches toggle WIFI flags.
 */
#if (defined(ESP32) || defined(ESP8266))
  #define INTEGRALL_NETWORK_AVAILABLE 1
  #include <ArduinoJson.h> 
  #if defined(ESP32)
    #include <WiFi.h>
    #include <HTTPClient.h>
    #include <WiFiClient.h>
  #else
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
    #include <WiFiClient.h>
  #endif
  #if __has_include(<WiFiClientSecure.h>)
    #include <WiFiClientSecure.h>
    #define INTEGRALL_SECURE_AVAILABLE 1
  #else
    #define INTEGRALL_SECURE_AVAILABLE 0
  #endif
#else
  #define INTEGRALL_NETWORK_AVAILABLE 0
  #define INTEGRALL_SECURE_AVAILABLE 0
#endif

namespace Integrall {

// Connection states for state machine
enum class DeviceState {
    UNINITIALIZED,      // Just created, not configured
    CONFIGURING,        // In config portal (WiFiManager / SoftAP mode)
    DISCONNECTED,       // WiFi configured but not connected
    CONNECTING_WIFI,    // Attempting WiFi connection
    WIFI_CONNECTED,     // WiFi up, not yet registered with backend
    REGISTERING,        // Sending registration to backend
    ONLINE,             // Fully operational
    ERROR               // Unrecoverable error state
};

// Configuration structure
struct DeviceConfig {
    const char* wifi_ssid = nullptr;
    const char* wifi_password = nullptr;
    const char* backend_url = nullptr;      // e.g., "http://192.168.1.100:8000" or "https://api.example.com"
    const char* api_key = nullptr;
    uint16_t backend_port = 8000;
    bool use_wifi_manager = false;          // If true, launch captive portal / AP
    uint32_t wifi_connect_timeout_ms = 10000;
    uint32_t reconnect_interval_ms = 5000;
    uint32_t poll_interval_ms = 3000;
};

#if INTEGRALL_NETWORK_AVAILABLE
/**
 * Structured HTTP/HTTPS response object
 */
struct HttpResponse {
    int    statusCode = -1;
    String body;
    String contentType;
    int    contentLength = 0;

    bool ok() const { return statusCode >= 200 && statusCode < 300; }
};

struct HttpHeaderKV {
    String key;
    String value;
};
#endif

class DeviceManager {
public:
    DeviceManager();
    
    // Initialize with configuration
    bool begin(const DeviceConfig& config);
    
    // Main loop - call frequently, returns immediately
    void handle();
    
    // State queries
    DeviceState getState() const { return _state; }
    bool isOnline() const { return _state == DeviceState::ONLINE; }
    
    bool isWiFiConnected() const { 
        #if INTEGRALL_NETWORK_AVAILABLE
        return WiFi.status() == WL_CONNECTED; 
        #else
        return false;
        #endif
    }
    
    // Device identity
    const char* getDeviceId() const { return _device_id; }
    const char* getDeviceIp() const { return _device_ip; }
    
    int getWiFiStrength() const { 
        #if INTEGRALL_NETWORK_AVAILABLE
        return WiFi.RSSI(); 
        #else
        return 0;
        #endif
    }
    
    // Backend & HTTP/HTTPS REST Client
    #if INTEGRALL_NETWORK_AVAILABLE
    bool sendTelemetry(const JsonDocument& data);
    bool sendCommandResponse(const char* command_id, bool success, const char* message = nullptr);
    
    // Custom HTTP/HTTPS Headers, Auth & TLS Configuration
    void setHttpHeader(const char* key, const char* value);
    void clearHttpHeaders();
    void setBearerToken(const char* token);
    void setCACert(const char* rootCA);
    void setHttpInsecure(bool insecure = true);
    void setHttpTimeout(uint16_t timeoutMs);

    // Full HTTP/HTTPS Request API (Auto-detects http:// vs https://)
    HttpResponse httpRequest(const char* method, const char* url, const char* payload = nullptr, const char* contentType = "application/json", const char* authToken = nullptr);
    String httpGet(const char* url, const char* authToken = nullptr);
    int httpPost(const char* url, const char* payload, const char* contentType = "application/json", const char* authToken = nullptr);
    int httpPut(const char* url, const char* payload, const char* contentType = "application/json", const char* authToken = nullptr);
    int httpPatch(const char* url, const char* payload, const char* contentType = "application/json", const char* authToken = nullptr);
    int httpDelete(const char* url, const char* payload = nullptr, const char* authToken = nullptr);
    
    /**
     * Post a file/buffer as multipart form data (supports both HTTP and HTTPS)
     */
    int httpPostFile(const char* url, uint8_t* data, size_t len, const char* fileName, const char* fileKey = "file", const char* extraKey = nullptr, const char* extraVal = nullptr);

    /**
     * Get a stream for the given URL (caller must end the request)
     */
    Stream* getStream(const char* url);
    void endStream();
    #endif
    
    // Configuration portal (SoftAP / Hotspot)
    bool startConfigPortal(const char* ap_name = "Integrall-Setup", 
                          const char* ap_password = nullptr);
    bool stopConfigPortal();
    
    // Force reconnection
    void reconnect();
    
    // Get last error, status code, or response body
    const char* getLastError() const { return _last_error; }
    const char* getLastResponse() const { return _last_response.c_str(); }
    int getLastStatusCode() const { return _last_status_code; }

private:
    DeviceConfig _config;
    DeviceState _state;
    char _device_id[32];
    char _device_ip[16];
    char _last_error[64];
    bool _wifi_configured;
    
    // Timing
    unsigned long _last_reconnect_attempt;
    unsigned long _last_register_attempt;
    unsigned long _registration_retry_count;
    
    // HTTP / HTTPS client state
    #if INTEGRALL_NETWORK_AVAILABLE
    HTTPClient _http;
    WiFiClient _wifiClient;
      #if INTEGRALL_SECURE_AVAILABLE
      WiFiClientSecure _secureClient;
      #endif
    static const uint8_t MAX_CUSTOM_HEADERS = 6;
    HttpHeaderKV _custom_headers[MAX_CUSTOM_HEADERS];
    uint8_t      _custom_header_count = 0;
    String       _bearer_token;
    const char*  _root_ca = nullptr;
    bool         _tls_insecure = true;
    uint16_t     _http_timeout_ms = 10000;

    bool _beginHttpUrl(const char* url);
    void _applyCustomHeaders(const char* authToken = nullptr);
    #endif
    
    // Internal methods
    void _generateDeviceId();
    bool _connectWiFi();
    void _handleWiFiReconnect();
    bool _registerWithBackend();
    void _setState(DeviceState new_state);
    void _setError(const char* error);
    void _updateIpFromWiFi();
    
    String _last_response;
    int    _last_status_code = 0;
};

} // namespace Integrall

#endif // INTEGRALL_DEVICE_MANAGER_H
