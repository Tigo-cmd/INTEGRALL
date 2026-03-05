/**
 * DeviceManager.cpp
 * 
 * Implementation of device management core
 */

#include "DeviceManager.h"

namespace Integrall {

// Static instance pointer for WiFi event callbacks
DeviceManager* DeviceManager::_instance = nullptr;

DeviceManager::DeviceManager() 
    : _state(DeviceState::UNINITIALIZED)
    , _last_reconnect_attempt(0)
    , _last_poll_time(0)
    , _registration_retry_count(0) {
    _device_id[0] = '\0';
    _device_ip[0] = '\0';
    _last_error[0] = '\0';
}

bool DeviceManager::begin(const DeviceConfig& config) {
    _config = config;
    _instance = this;  // Set static pointer for callbacks
    
    Logger::begin();
    INTEGRALL_LOG_INFO("Integrall DeviceManager v" INTEGRALL_VERSION_STRING);
    
    // Generate unique device ID from ESP32 MAC address
    _generateDeviceId();
    INTEGRALL_LOG_INFO_VAL("Device ID: ", _device_id);
    
    // Register WiFi event handlers (event-driven, no polling)
    WiFi.onEvent(_onWiFiConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(_onWiFiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(_onWiFiConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    
    // Attempt WiFi connection if credentials provided
    if (_config.wifi_ssid && strlen(_config.wifi_ssid) > 0) {
        _setState(DeviceState::CONNECTING_WIFI);
        if (!_connectWiFi()) {
            if (_config.use_wifi_manager) {
                return startConfigPortal();
            } else {
                _setState(DeviceState::DISCONNECTED);
                _setError("WiFi connection failed");
                return false;
            }
        }
    } else if (_config.use_wifi_manager) {
        // No credentials, start config portal immediately
        return startConfigPortal();
    } else {
        _setError("No WiFi credentials provided");
        return false;
    }
    
    return true;
}

void DeviceManager::handle() {
    switch (_state) {
        case DeviceState::DISCONNECTED:
            _handleWiFiReconnect();
            break;
            
        case DeviceState::WIFI_CONNECTED:
            // WiFi up but not registered - try to register
            if (_config.backend_url) {
                _setState(DeviceState::REGISTERING);
            }
            break;
            
        case DeviceState::REGISTERING:
            if (_registerWithBackend()) {
                _setState(DeviceState::ONLINE);
                _registration_retry_count = 0;
            } else {
                _registration_retry_count++;
                if (_registration_retry_count >= INTEGRALL_HTTP_RETRY_ATTEMPTS) {
                    _setError("Backend registration failed after retries");
                    _setState(DeviceState::ERROR);
                }
                // Back off before retry
                delay(1000);
            }
            break;
            
        case DeviceState::ONLINE:
            // Check for backend commands periodically
            if (millis() - _last_poll_time >= _config.poll_interval_ms) {
                _checkBackendCommands();
                _last_poll_time = millis();
            }
            break;
            
        case DeviceState::CONFIGURING:
            // Config portal is blocking in current implementation
            // Non-blocking version would be handled here
            break;
            
        default:
            break;
    }
}

bool DeviceManager::_connectWiFi() {
    if (!_config.wifi_ssid || strlen(_config.wifi_ssid) == 0) {
        return false;
    }
    
    INTEGRALL_LOG_INFO_VAL("Connecting to WiFi: ", _config.wifi_ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(_device_id);
    
    // Use persistent credentials if available (avoids flash wear)
    if (WiFi.SSID() != String(_config.wifi_ssid)) {
        WiFi.begin(_config.wifi_ssid, _config.wifi_password);
    } else {
        // Credentials already in flash, just reconnect
        WiFi.reconnect();
    }
    
    // Wait for connection (blocking in setup, non-blocking in loop)
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && 
           (millis() - start < _config.wifi_connect_timeout_ms)) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        INTEGRALL_LOG_INFO_VAL("WiFi connected, IP: ", WiFi.localIP().toString().c_str());
        strncpy(_device_ip, WiFi.localIP().toString().c_str(), sizeof(_device_ip) - 1);
        _device_ip[sizeof(_device_ip) - 1] = '\0';
        return true;
    }
    
    INTEGRALL_LOG_ERROR("WiFi connection timeout");
    return false;
}

void DeviceManager::_handleWiFiReconnect() {
    unsigned long now = millis();
    if (now - _last_reconnect_attempt >= _config.reconnect_interval_ms) {
        _last_reconnect_attempt = now;
        INTEGRALL_LOG_INFO("Attempting WiFi reconnect...");
        WiFi.reconnect();
    }
}

bool DeviceManager::_registerWithBackend() {
    if (!_config.backend_url || !_config.api_key) {
        return false;
    }
    
    char url[128];
    snprintf(url, sizeof(url), "%s/api/devices/register", _config.backend_url);
    
    INTEGRALL_LOG_INFO_VAL("Registering with backend: ", url);
    
    StaticJsonDocument<512> doc;
    doc["device_id"] = _device_id;
    doc["ip_address"] = _device_ip;
    doc["mac_address"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    doc["firmware_version"] = INTEGRALL_VERSION_STRING;
    doc["sdk_version"] = ESP.getSdkVersion();
    
    char payload[512];
    serializeJson(doc, payload);
    
    _http.begin(url);
    _http.addHeader("Content-Type", "application/json");
    _http.addHeader("X-API-Key", _config.api_key);
    _http.setTimeout(INTEGRALL_HTTP_TIMEOUT_MS);
    
    int httpCode = _http.POST(payload);
    bool success = (httpCode == 200 || httpCode == 201);
    
    if (success) {
        INTEGRALL_LOG_INFO("Backend registration successful");
        // Parse response for any server-side config
        StaticJsonDocument<256> response;
        deserializeJson(response, _http.getString());
        // Could extract server config here if needed
    } else {
        INTEGRALL_LOG_ERROR_VAL("Registration failed, HTTP code: ", httpCode);
    }
    
    _http.end();
    return success;
}

void DeviceManager::_checkBackendCommands() {
    if (!_config.backend_url || !_config.api_key) {
        return;
    }
    
    char url[128];
    snprintf(url, sizeof(url), "%s/api/devices/%s/commands", 
             _config.backend_url, _device_id);
    
    _http.begin(url);
    _http.addHeader("X-API-Key", _config.api_key);
    _http.setTimeout(2000);  // Short timeout for polling
    
    int httpCode = _http.GET();
    
    if (httpCode == 200) {
        String response = _http.getString();
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error && doc.containsKey("commands")) {
            JsonArray commands = doc["commands"];
            for (JsonObject cmd : commands) {
                const char* cmd_type = cmd["command"];
                INTEGRALL_LOG_INFO_VAL("Received command: ", cmd_type);
                // Commands will be dispatched to modules via Integrall main class
            }
        }
    }
    
    _http.end();
}

void DeviceManager::_generateDeviceId() {
    // Use ESP32 MAC address to create unique ID
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(_device_id, sizeof(_device_id), 
             "INT_%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void DeviceManager::_setState(DeviceState new_state) {
    if (_state != new_state) {
        _state = new_state;
        // Could emit event here for state changes
    }
}

void DeviceManager::_setError(const char* error) {
    INTEGRALL_SAFE_STRCPY(_last_error, error, sizeof(_last_error));
    INTEGRALL_LOG_ERROR_VAL("Error: ", error);
}

// Static WiFi event handlers
void DeviceManager::_onWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (_instance) {
        if (event == WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP) {
            INTEGRALL_LOG_INFO("WiFi event: Got IP");
            strncpy(_instance->_device_ip, 
                   WiFi.localIP().toString().c_str(), 
                   sizeof(_instance->_device_ip) - 1);
            _instance->_device_ip[sizeof(_instance->_device_ip) - 1] = '\0';
            _instance->_setState(DeviceState::WIFI_CONNECTED);
        }
    }
}

void DeviceManager::_onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (_instance) {
        INTEGRALL_LOG_WARN("WiFi event: Disconnected");
        _instance->_setState(DeviceState::DISCONNECTED);
        _instance->_last_reconnect_attempt = millis();  // Reset reconnect timer
    }
}

bool DeviceManager::sendTelemetry(const JsonDocument& data) {
    if (_state != DeviceState::ONLINE) {
        return false;
    }
    
    char url[128];
    snprintf(url, sizeof(url), "%s/api/telemetry", _config.backend_url);
    
    StaticJsonDocument<1024> doc;
    doc["device_id"] = _device_id;
    doc["data"] = data;
    
    char payload[1024];
    serializeJson(doc, payload);
    
    _http.begin(url);
    _http.addHeader("Content-Type", "application/json");
    _http.addHeader("X-API-Key", _config.api_key);
    
    int httpCode = _http.POST(payload);
    _http.end();
    
    return (httpCode == 200 || httpCode == 201);
}

bool DeviceManager::sendCommandResponse(const char* command_id, bool success, const char* message) {
    if (_state != DeviceState::ONLINE) {
        return false;
    }
    
    char url[128];
    snprintf(url, sizeof(url), "%s/api/commands/%s/response", _config.backend_url, command_id);
    
    StaticJsonDocument<256> doc;
    doc["success"] = success;
    if (message) {
        doc["message"] = message;
    }
    
    char payload[256];
    serializeJson(doc, payload);
    
    _http.begin(url);
    _http.addHeader("Content-Type", "application/json");
    _http.addHeader("X-API-Key", _config.api_key);
    
    int httpCode = _http.POST(payload);
    _http.end();
    
    return (httpCode == 200 || httpCode == 201);
}

bool DeviceManager::startConfigPortal(const char* ap_name, const char* ap_password) {
    // TODO: Integrate WiFiManager library here
    // For now, mark as configuring state
    _setState(DeviceState::CONFIGURING);
    INTEGRALL_LOG_INFO("Starting config portal...");
    INTEGRALL_LOG_WARN("WiFiManager integration required - using fallback");
    
    // Fallback: Create open AP with simple web server
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_name, ap_password);
    
    INTEGRALL_LOG_INFO_VAL("Config portal started. Connect to: ", ap_name);
    INTEGRALL_LOG_INFO_VAL("Portal IP: ", WiFi.softAPIP().toString().c_str());
    
    return true;
}

bool DeviceManager::stopConfigPortal() {
    WiFi.softAPdisconnect(true);
    _setState(DeviceState::DISCONNECTED);
    return true;
}

void DeviceManager::reconnect() {
    if (_state == DeviceState::DISCONNECTED || 
        _state == DeviceState::ERROR) {
        _setState(DeviceState::CONNECTING_WIFI);
        WiFi.reconnect();
    }
}

} // namespace Integrall
