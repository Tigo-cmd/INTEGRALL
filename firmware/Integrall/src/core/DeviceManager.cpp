/**
 * DeviceManager.cpp
 * 
 * Implementation of device management core
 */

// Force compilation of networking logic in the library object file.
// The Arduino compilation process builds .cpp files independently of the sketch.
// By defining this here, we ensure the linker has the network symbols available.
// If the user's sketch DOES NOT enable WiFi, the linker will simply garbage-collect (--gc-sections)
// this unused code, resulting in zero overhead.
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
   #ifndef INTEGRALL_VERSION_STRING
  #define INTEGRALL_VERSION_STRING "1.0.0"
#endif

#ifndef INTEGRALL_BACKEND_ENABLED
  #define INTEGRALL_BACKEND_ENABLED 1
#endif
    INTEGRALL_LOG_INFO("Integrall System Starting...");
    INTEGRALL_LOG_INFO_VAL("Version: ", INTEGRALL_VERSION_STRING);
    
    // Generate unique device ID
    _generateDeviceId();
    INTEGRALL_LOG_INFO_VAL("Device ID: ", _device_id);
    
    #if INTEGRALL_NETWORK_AVAILABLE
        #if defined(ESP32)
        // Register WiFi event handlers (event-driven, no polling)
        WiFi.onEvent(_onWiFiConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
        WiFi.onEvent(_onWiFiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        WiFi.onEvent(_onWiFiConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
        #endif

        // Attempt WiFi connection if credentials provided
        if (_config.wifi_ssid && strlen(_config.wifi_ssid) > 0) {
            if (_config.use_wifi_manager) {
                // Explicit AP Mode requested
                return startConfigPortal(_config.wifi_ssid, _config.wifi_password);
            } else {
                // Standard Station (STA) Mode
                _setState(DeviceState::CONNECTING_WIFI);
                if (!_connectWiFi()) {
                    _setState(DeviceState::DISCONNECTED);
                    _setError("WiFi connection failed");
                    return false;
                }
            }
        } else if (_config.use_wifi_manager) {
            return startConfigPortal();
        }
    #endif
    
    return true;
}

void DeviceManager::handle() {
    #if INTEGRALL_NETWORK_AVAILABLE
    switch (_state) {
        case DeviceState::DISCONNECTED:
            _handleWiFiReconnect();
            break;
            
        case DeviceState::WIFI_CONNECTED:
            #if INTEGRALL_BACKEND_ENABLED
            if (_config.backend_url) {
                _setState(DeviceState::REGISTERING);
            } else {
                _setState(DeviceState::ONLINE);
            }
            #else
            _setState(DeviceState::ONLINE);
            #endif
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
                delay(1000);
            }
            break;
            
        case DeviceState::ONLINE:
            #if INTEGRALL_BACKEND_ENABLED
            if (millis() - _last_poll_time >= _config.poll_interval_ms) {
                _checkBackendCommands();
                _last_poll_time = millis();
            }
            #endif
            break;
            
        default:
            break;
    }
    #endif
}

bool DeviceManager::_connectWiFi() {
    #if INTEGRALL_NETWORK_AVAILABLE
    if (!_config.wifi_ssid || strlen(_config.wifi_ssid) == 0) {
        return false;
    }
    
    INTEGRALL_LOG_INFO_VAL("Connecting to WiFi: ", _config.wifi_ssid);
    
    WiFi.mode(WIFI_STA);
    #if defined(ESP32)
    WiFi.setHostname(_device_id);
    #endif
    
    WiFi.begin(_config.wifi_ssid, _config.wifi_password);
    
    // Wait for connection
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
    #endif
    return false;
}

void DeviceManager::_handleWiFiReconnect() {
    #if INTEGRALL_NETWORK_AVAILABLE
    unsigned long now = millis();
    if (now - _last_reconnect_attempt >= _config.reconnect_interval_ms) {
        _last_reconnect_attempt = now;
        INTEGRALL_LOG_INFO("Attempting WiFi reconnect...");
        WiFi.reconnect();
    }
    #endif
}

bool DeviceManager::_registerWithBackend() {
    #if INTEGRALL_NETWORK_AVAILABLE && INTEGRALL_BACKEND_ENABLED
    if (!_config.backend_url || !_config.api_key) {
        return false;
    }
    
    char url[128];
    snprintf(url, sizeof(url), "%s/api/devices/register", _config.backend_url);
    
    StaticJsonDocument<512> doc;
    doc["device_id"] = _device_id;
    doc["ip_address"] = _device_ip;
    
    #if INTEGRALL_NETWORK_AVAILABLE
    doc["mac_address"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    doc["sdk_version"] = ESP.getSdkVersion();
    #endif
    doc["firmware_version"] = INTEGRALL_VERSION_STRING;
    
    char payload[512];
    serializeJson(doc, payload);
    
    #if defined(ESP32)
    _http.begin(url);
    #else
    _http.begin(_wifiClient, url);
    #endif

    _http.addHeader("Content-Type", "application/json");
    _http.addHeader("X-API-Key", _config.api_key);
    
    int httpCode = _http.POST(payload);
    bool success = (httpCode == 200 || httpCode == 201);
    _http.end();
    return success;
    #endif
    return false;
}

void DeviceManager::_checkBackendCommands() {
    #if INTEGRALL_NETWORK_AVAILABLE && INTEGRALL_BACKEND_ENABLED
    if (!_config.backend_url || !_config.api_key) return;
    
    char url[128];
    snprintf(url, sizeof(url), "%s/api/devices/%s/commands", _config.backend_url, _device_id);
    
    #if defined(ESP32)
    _http.begin(url);
    #else
    _http.begin(_wifiClient, url);
    #endif

    _http.addHeader("X-API-Key", _config.api_key);
    
    int httpCode = _http.GET();
    if (httpCode == 200) {
        String response = _http.getString();
        StaticJsonDocument<512> doc;
        if (!deserializeJson(doc, response)) {
            // Processing logic would go here
        }
    }
    _http.end();
    #endif
}

void DeviceManager::_generateDeviceId() {
    #if INTEGRALL_NETWORK_AVAILABLE
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(_device_id, sizeof(_device_id), "INT_%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    #else
    snprintf(_device_id, sizeof(_device_id), "INT_OFFLINE_%08X", (unsigned int)millis());
    #endif
}

void DeviceManager::_setState(DeviceState new_state) {
    _state = new_state;
}

void DeviceManager::_setError(const char* error) {
    INTEGRALL_SAFE_STRCPY(_last_error, error, sizeof(_last_error));
}

#if defined(ESP32) && INTEGRALL_NETWORK_AVAILABLE
void DeviceManager::_onWiFiConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (_instance && event == WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP) {
        strncpy(_instance->_device_ip, WiFi.localIP().toString().c_str(), sizeof(_instance->_device_ip) - 1);
        _instance->_setState(DeviceState::WIFI_CONNECTED);
    }
}
void DeviceManager::_onWiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (_instance) _instance->_setState(DeviceState::DISCONNECTED);
}
#endif

#if INTEGRALL_NETWORK_AVAILABLE
bool DeviceManager::sendTelemetry(const JsonDocument& data) {
    #if INTEGRALL_BACKEND_ENABLED
    // Allow sending data if we have WiFi, even if registration isn't complete yet
    // (Crucial for simple Flask/Community backends that only want JSON posts)
    if (_state != DeviceState::ONLINE && _state != DeviceState::WIFI_CONNECTED) {
        return false;
    }
    
    char url[128];
    snprintf(url, sizeof(url), "%s/api/telemetry", _config.backend_url);
    
    StaticJsonDocument<1024> doc;
    doc["device_id"] = _device_id;
    doc["data"] = data;
    
    char payload[1024];
    serializeJson(doc, payload);
    
    #if defined(ESP32)
    _http.begin(url);
    #else
    _http.begin(_wifiClient, url);
    #endif

    _http.addHeader("Content-Type", "application/json");
    _http.addHeader("X-API-Key", _config.api_key);
    
    int httpCode = _http.POST(payload);
    _http.end();
    return (httpCode == 200 || httpCode == 201);
    #endif
    return false;
}
#endif

#if INTEGRALL_NETWORK_AVAILABLE
bool DeviceManager::sendCommandResponse(const char* command_id, bool success, const char* message) {
    #if INTEGRALL_BACKEND_ENABLED
    if (_state != DeviceState::ONLINE) return false;
    
    char url[128];
    snprintf(url, sizeof(url), "%s/api/commands/%s/response", _config.backend_url, command_id);
    
    StaticJsonDocument<256> doc;
    doc["success"] = success;
    if (message) doc["message"] = message;
    
    #if defined(ESP32)
    _http.begin(url);
    #else
    _http.begin(_wifiClient, url);
    #endif
    
    _http.addHeader("Content-Type", "application/json");
    _http.addHeader("X-API-Key", _config.api_key);
    
    String payload;
    serializeJson(doc, payload);
    int httpCode = _http.POST(payload);
    _http.end();
    return (httpCode == 200 || httpCode == 201);
    #endif
    return false;
}
#endif

#if INTEGRALL_NETWORK_AVAILABLE
bool DeviceManager::startConfigPortal(const char* ap_name, const char* ap_password) {
    _setState(DeviceState::CONFIGURING);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_name, ap_password);
    return true;
}
#endif

#if INTEGRALL_NETWORK_AVAILABLE
bool DeviceManager::stopConfigPortal() {
    WiFi.softAPdisconnect(true);
    _setState(DeviceState::DISCONNECTED);
    return true;
}
#endif

void DeviceManager::reconnect() {
    #if INTEGRALL_NETWORK_AVAILABLE
    if (_state == DeviceState::DISCONNECTED || _state == DeviceState::ERROR) {
        _setState(DeviceState::CONNECTING_WIFI);
        WiFi.reconnect();
    }
    #endif
}

#if INTEGRALL_NETWORK_AVAILABLE
String DeviceManager::httpGet(const char* url) {
    #if defined(ESP32)
    _http.begin(url);
    #else
    _http.begin(_wifiClient, url);
    #endif
    
    int code = _http.GET();
    _last_response = "";
    if (code == 200) _last_response = _http.getString();
    _http.end();
    return _last_response;
}

int DeviceManager::httpPost(const char* url, const char* payload, const char* contentType) {
    #if defined(ESP32)
    _http.begin(url);
    #else
    _http.begin(_wifiClient, url);
    #endif
    
    _http.addHeader("Content-Type", contentType);
    int code = _http.POST(payload);
    _last_response = "";
    if (code > 0) _last_response = _http.getString();
    _http.end();
    return code;
}

int DeviceManager::httpPostFile(const char* url, uint8_t* data, size_t len, const char* fileName, const char* fileKey, const char* extraKey, const char* extraVal) {
    #if defined(ESP32)
    _http.begin(url);
    #else
    _http.begin(_wifiClient, url);
    #endif

    String boundary = "----IntegrallBoundary" + String(millis());
    _http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    
    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"" + String(fileKey) + "\"; filename=\"" + String(fileName) + "\"\r\n";
    head += "Content-Type: application/octet-stream\r\n\r\n";
    
    String tail = "\r\n--" + boundary;
    if (extraKey && extraVal) {
        tail += "\r\nContent-Disposition: form-data; name=\"" + String(extraKey) + "\"\r\n\r\n" + String(extraVal) + "\r\n--" + boundary;
    }
    tail += "--\r\n";
    
    size_t totalLen = head.length() + len + tail.length();
    _http.addHeader("Content-Length", String(totalLen));
    
    int code = _http.sendRequest("POST", (uint8_t*)head.c_str(), head.length());
    if (code > 0) {
        uint8_t* buffer = (uint8_t*)malloc(totalLen);
        if (buffer) {
            memcpy(buffer, head.c_str(), head.length());
            memcpy(buffer + head.length(), data, len);
            memcpy(buffer + head.length() + len, tail.c_str(), tail.length());
            code = _http.POST(buffer, totalLen);
            free(buffer);
        } else {
            code = -2; // Out of memory
        }
    }
    
    _last_response = "";
    if (code > 0) _last_response = _http.getString();
    _http.end();
    return code;
}

Stream* DeviceManager::getStream(const char* url) {
    #if defined(ESP32)
    _http.begin(url);
    #else
    _http.begin(_wifiClient, url);
    #endif
    
    int code = _http.GET();
    if (code == 200) {
        #if defined(ESP32)
        return &_http.getStream();
        #else
        return &_wifiClient;
        #endif
    }
    return nullptr;
}

void DeviceManager::endStream() {
    _http.end();
}
#endif

} // namespace Integrall
