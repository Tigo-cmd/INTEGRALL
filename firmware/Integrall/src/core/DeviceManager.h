/**
 * DeviceManager.h
 * 
 * Core device management for Integrall Framework
 * Handles WiFi connection, backend registration, and device identity
 * 
 * Features:
 * - Event-driven WiFi reconnection (no polling delays)
 * - Persistent device ID (ESP32 chip ID based)
 * - Backend registration with retry logic
 * - Non-blocking operation (returns control immediately)
 * - Optional WiFiManager integration ready
 */

#ifndef INTEGRALL_DEVICE_MANAGER_H
#define INTEGRALL_DEVICE_MANAGER_H

#include "../config/IntegrallConfig.h"
#include "Logger.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace Integrall {

// Connection states for state machine
enum class DeviceState {
    UNINITIALIZED,      // Just created, not configured
    CONFIGURING,        // In config portal (WiFiManager mode)
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
    const char* backend_url = nullptr;      // e.g., "http://192.168.1.100:8000"
    const char* api_key = nullptr;
    uint16_t backend_port = 8000;
    bool use_wifi_manager = false;          // If true, launch captive portal on fail
    uint32_t wifi_connect_timeout_ms = 10000;
    uint32_t reconnect_interval_ms = 5000;
    uint32_t poll_interval_ms = 3000;
};

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
    bool isWiFiConnected() const { return WiFi.status() == WL_CONNECTED; }
    
    // Device identity
    const char* getDeviceId() const { return _device_id; }
    const char* getDeviceIp() const { return _device_ip; }
    int getWiFiRSSI() const { return WiFi.RSSI(); }
    
    // Backend communication
    bool sendTelemetry(const JsonDocument& data);
    bool sendCommandResponse(const char* command_id, bool success, const char* message = nullptr);
    
    // Configuration portal (WiFiManager integration point)
    bool startConfigPortal(const char* ap_name = "Integrall-Setup", 
                          const char* ap_password = nullptr);
    bool stopConfigPortal();
    
    // Force reconnection
    void reconnect();
    
    // Get last error
    const char* getLastError() const { return _last_error; }

private:
    DeviceConfig _config;
    DeviceState _state;
    char _device_id[32];
    char _device_ip[16];
    char _last_error[64];
    
    // Timing
    unsigned long _last_reconnect_attempt;
    unsigned long _last_poll_time;
    unsigned long _registration_retry_count;
    
    // HTTP client
    HTTPClient _http;
    
    // Internal methods
    void _generateDeviceId();
    bool _connectWiFi();
    void _handleWiFiReconnect();
    bool _registerWithBackend();
    void _checkBackendCommands();
    void _setState(DeviceState new_state);
    void _setError(const char* error);
    
    // WiFi event handlers (static because WiFi.onEvent requires static)
    static void _onWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t info);
    static void _onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
    static DeviceManager* _instance;  // Singleton pointer for callbacks
};

} // namespace Integrall

#endif // INTEGRALL_DEVICE_MANAGER_H
