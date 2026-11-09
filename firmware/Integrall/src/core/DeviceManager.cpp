/**
 * DeviceManager.cpp
 * 
 * Implementation of device management core & HTTP/HTTPS REST client
 */

#include "DeviceManager.h"

#if INTEGRALL_NETWORK_AVAILABLE
  #if ARDUINOJSON_VERSION_MAJOR >= 7
    #define INTEGRALL_JSON_DOC(name, cap) JsonDocument name
  #else
    #define INTEGRALL_JSON_DOC(name, cap) StaticJsonDocument<cap> name
  #endif
#endif

namespace Integrall {

DeviceManager::DeviceManager() 
    : _state(DeviceState::UNINITIALIZED)
    , _wifi_configured(false)
    , _last_reconnect_attempt(0)
    , _last_register_attempt(0)
    , _registration_retry_count(0) {
    _device_id[0] = '\0';
    _device_ip[0] = '\0';
    _last_error[0] = '\0';
}

bool DeviceManager::begin(const DeviceConfig& config) {
    _config = config;
    
    Logger::begin();
    #ifndef INTEGRALL_VERSION_STRING
      #define INTEGRALL_VERSION_STRING "1.1.0"
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
        // Attempt WiFi connection if credentials provided
        if (_config.wifi_ssid && strlen(_config.wifi_ssid) > 0) {
            if (_config.use_wifi_manager) {
                // Explicit AP Mode requested
                return startConfigPortal(_config.wifi_ssid, _config.wifi_password);
            } else {
                // Standard Station (STA) Mode
                _wifi_configured = true;
                _setState(DeviceState::CONNECTING_WIFI);
                if (_connectWiFi()) {
                    _setState(DeviceState::WIFI_CONNECTED);
                } else {
                    _setState(DeviceState::DISCONNECTED);
                    _setError("WiFi initial connection timed out; will retry in background");
                    _last_reconnect_attempt = millis();
                    return false;
                }
            }
        } else if (_config.use_wifi_manager) {
            return startConfigPortal();
        }
    #endif
    
    return true;
}

void DeviceManager::_updateIpFromWiFi() {
    #if INTEGRALL_NETWORK_AVAILABLE
    String ip = WiFi.localIP().toString();
    INTEGRALL_SAFE_STRCPY(_device_ip, ip.c_str(), sizeof(_device_ip));
    #endif
}

void DeviceManager::handle() {
    #if INTEGRALL_NETWORK_AVAILABLE
    if (!_wifi_configured && _state != DeviceState::CONFIGURING) {
        return;
    }

    if (_state != DeviceState::CONFIGURING) {
        bool connected = (WiFi.status() == WL_CONNECTED);
        if (!connected && (_state == DeviceState::WIFI_CONNECTED ||
                           _state == DeviceState::REGISTERING ||
                           _state == DeviceState::ONLINE)) {
            INTEGRALL_LOG_WARN("WiFi connection lost");
            _setState(DeviceState::DISCONNECTED);
            _last_reconnect_attempt = millis();
        } else if (connected && (_state == DeviceState::DISCONNECTED ||
                                _state == DeviceState::CONNECTING_WIFI)) {
            _updateIpFromWiFi();
            INTEGRALL_LOG_INFO_VAL("WiFi reconnected, IP: ", _device_ip);
            _registration_retry_count = 0;
            _setState(DeviceState::WIFI_CONNECTED);
        }
    }

    switch (_state) {
        case DeviceState::DISCONNECTED:
        case DeviceState::CONNECTING_WIFI:
            _handleWiFiReconnect();
            break;
            
        case DeviceState::WIFI_CONNECTED:
            #if INTEGRALL_BACKEND_ENABLED
            if (_config.backend_url && strlen(_config.backend_url) > 0 &&
                _config.api_key && strlen(_config.api_key) > 0) {
                _last_register_attempt = 0;
                _setState(DeviceState::REGISTERING);
            } else {
                _setState(DeviceState::ONLINE);
            }
            #else
            _setState(DeviceState::ONLINE);
            #endif
            break;
            
        case DeviceState::REGISTERING:
            if (_last_register_attempt == 0 || (millis() - _last_register_attempt >= 1000)) {
                _last_register_attempt = millis();
                if (_registerWithBackend()) {
                    _setState(DeviceState::ONLINE);
                    _registration_retry_count = 0;
                } else {
                    _registration_retry_count++;
                    if (_registration_retry_count >= INTEGRALL_HTTP_RETRY_ATTEMPTS) {
                        _setError("Backend registration failed; continuing online without registration");
                        INTEGRALL_LOG_WARN("Backend registration failed after retries; proceeding to ONLINE state");
                        _setState(DeviceState::ONLINE);
                    }
                }
            }
            break;
            
        case DeviceState::ONLINE:
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
    #elif defined(ESP8266)
    WiFi.hostname(_device_id);
    #endif
    
    WiFi.begin(_config.wifi_ssid, _config.wifi_password);
    
    // Wait for connection
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && 
           (millis() - start < _config.wifi_connect_timeout_ms)) {
        delay(100);
        #if INTEGRALL_DEBUG_LEVEL > 0
        if (Logger::ready()) Serial.print(".");
        #endif
    }
    #if INTEGRALL_DEBUG_LEVEL > 0
    if (Logger::ready()) Serial.println();
    #endif
    
    if (WiFi.status() == WL_CONNECTED) {
        _updateIpFromWiFi();
        INTEGRALL_LOG_INFO_VAL("WiFi connected, IP: ", _device_ip);
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
        _setState(DeviceState::CONNECTING_WIFI);
        WiFi.reconnect();
    }
    #endif
}

bool DeviceManager::_registerWithBackend() {
    #if INTEGRALL_NETWORK_AVAILABLE && INTEGRALL_BACKEND_ENABLED
    if (!_config.backend_url || strlen(_config.backend_url) == 0 ||
        !_config.api_key || strlen(_config.api_key) == 0) {
        return false;
    }
    
    char url[192];
    snprintf(url, sizeof(url), "%s/api/devices/register", _config.backend_url);
    
    INTEGRALL_JSON_DOC(doc, 512);
    doc["device_id"] = _device_id;
    doc["ip_address"] = _device_ip;
    doc["mac_address"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    doc["sdk_version"] = ESP.getSdkVersion();
    doc["firmware_version"] = INTEGRALL_VERSION_STRING;
    
    char payload[512];
    serializeJson(doc, payload, sizeof(payload));
    
    if (!_beginHttpUrl(url)) return false;

    _http.addHeader("Content-Type", "application/json");
    _http.addHeader("X-API-Key", _config.api_key);
    
    int httpCode = _http.POST(payload);
    bool success = (httpCode == 200 || httpCode == 201);
    _http.end();
    return success;
    #else
    return false;
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
    if (error) {
        INTEGRALL_SAFE_STRCPY(_last_error, error, sizeof(_last_error));
    }
}

#if INTEGRALL_NETWORK_AVAILABLE
bool DeviceManager::_beginHttpUrl(const char* url) {
    if (!url || strlen(url) == 0) return false;
    _http.setTimeout(_http_timeout_ms);

    bool isHttps = (strncmp(url, "https://", 8) == 0 || strncmp(url, "HTTPS://", 8) == 0);
    if (isHttps) {
        #if INTEGRALL_SECURE_AVAILABLE
          #if defined(ESP32)
          if (_root_ca && !_tls_insecure) {
              _secureClient.setCACert(_root_ca);
          } else {
              _secureClient.setInsecure();
          }
          #else
          _secureClient.setInsecure();
          #endif
          return _http.begin(_secureClient, url);
        #else
          #if defined(ESP32)
          return _http.begin(url);
          #else
          return _http.begin(_wifiClient, url);
          #endif
        #endif
    } else {
        #if defined(ESP32)
        return _http.begin(_wifiClient, url);
        #else
        return _http.begin(_wifiClient, url);
        #endif
    }
}

void DeviceManager::_applyCustomHeaders(const char* authToken) {
    for (uint8_t i = 0; i < _custom_header_count; i++) {
        if (_custom_headers[i].key.length() > 0) {
            _http.addHeader(_custom_headers[i].key, _custom_headers[i].value);
        }
    }
    const char* tokenToUse = (authToken && strlen(authToken) > 0)
                             ? authToken
                             : (_bearer_token.length() > 0 ? _bearer_token.c_str() : nullptr);
    if (tokenToUse) {
        if (strncmp(tokenToUse, "Bearer ", 7) == 0 || strncmp(tokenToUse, "Basic ", 6) == 0) {
            _http.addHeader("Authorization", tokenToUse);
        } else {
            String authHdr = String("Bearer ") + tokenToUse;
            _http.addHeader("Authorization", authHdr);
        }
    }
}

void DeviceManager::setHttpHeader(const char* key, const char* value) {
    if (!key || strlen(key) == 0) return;
    // Update if key already exists
    for (uint8_t i = 0; i < _custom_header_count; i++) {
        if (_custom_headers[i].key.equalsIgnoreCase(key)) {
            _custom_headers[i].value = value ? value : "";
            return;
        }
    }
    if (_custom_header_count < MAX_CUSTOM_HEADERS) {
        _custom_headers[_custom_header_count].key   = key;
        _custom_headers[_custom_header_count].value = value ? value : "";
        _custom_header_count++;
    }
}

void DeviceManager::clearHttpHeaders() {
    _custom_header_count = 0;
}

void DeviceManager::setBearerToken(const char* token) {
    _bearer_token = token ? token : "";
}

void DeviceManager::setCACert(const char* rootCA) {
    _root_ca = rootCA;
    _tls_insecure = (rootCA == nullptr);
}

void DeviceManager::setHttpInsecure(bool insecure) {
    _tls_insecure = insecure;
}

void DeviceManager::setHttpTimeout(uint16_t timeoutMs) {
    _http_timeout_ms = timeoutMs > 0 ? timeoutMs : 10000;
}

bool DeviceManager::sendTelemetry(const JsonDocument& data) {
    #if INTEGRALL_BACKEND_ENABLED
    if (!isWiFiConnected() || !_config.backend_url || strlen(_config.backend_url) == 0) {
        return false;
    }
    
    char url[192];
    snprintf(url, sizeof(url), "%s/api/telemetry", _config.backend_url);
    
    INTEGRALL_JSON_DOC(doc, 1024);
    doc["device_id"] = _device_id;
    doc["data"] = data;
    
    String payload;
    serializeJson(doc, payload);
    
    if (!_beginHttpUrl(url)) return false;

    _http.addHeader("Content-Type", "application/json");
    if (_config.api_key && strlen(_config.api_key) > 0) {
        _http.addHeader("X-API-Key", _config.api_key);
    }
    
    int httpCode = _http.POST(payload);
    _http.end();
    return (httpCode >= 200 && httpCode < 300);
    #else
    (void)data;
    return false;
    #endif
}

bool DeviceManager::sendCommandResponse(const char* command_id, bool success, const char* message) {
    #if INTEGRALL_BACKEND_ENABLED
    if (!isWiFiConnected() || !_config.backend_url || strlen(_config.backend_url) == 0 || !command_id) {
        return false;
    }
    
    char url[192];
    snprintf(url, sizeof(url), "%s/api/commands/%s/response", _config.backend_url, command_id);
    
    INTEGRALL_JSON_DOC(doc, 256);
    doc["success"] = success;
    if (message) doc["message"] = message;
    
    if (!_beginHttpUrl(url)) return false;
    
    _http.addHeader("Content-Type", "application/json");
    if (_config.api_key && strlen(_config.api_key) > 0) {
        _http.addHeader("X-API-Key", _config.api_key);
    }
    
    String payload;
    serializeJson(doc, payload);
    int httpCode = _http.POST(payload);
    _http.end();
    return (httpCode >= 200 && httpCode < 300);
    #else
    (void)command_id;
    (void)success;
    (void)message;
    return false;
    #endif
}

bool DeviceManager::startConfigPortal(const char* ap_name, const char* ap_password) {
    _setState(DeviceState::CONFIGURING);
    WiFi.mode(WIFI_AP);
    bool ok = WiFi.softAP(ap_name ? ap_name : "Integrall-Setup", ap_password);
    if (ok) {
        String ip = WiFi.softAPIP().toString();
        INTEGRALL_SAFE_STRCPY(_device_ip, ip.c_str(), sizeof(_device_ip));
    }
    return ok;
}

bool DeviceManager::stopConfigPortal() {
    WiFi.softAPdisconnect(true);
    _setState(DeviceState::DISCONNECTED);
    return true;
}
#else
bool DeviceManager::startConfigPortal(const char* ap_name, const char* ap_password) {
    (void)ap_name;
    (void)ap_password;
    return false;
}

bool DeviceManager::stopConfigPortal() {
    return false;
}
#endif

void DeviceManager::reconnect() {
    #if INTEGRALL_NETWORK_AVAILABLE
    if (_wifi_configured) {
        _setState(DeviceState::CONNECTING_WIFI);
        _last_reconnect_attempt = millis();
        WiFi.reconnect();
    }
    #endif
}

#if INTEGRALL_NETWORK_AVAILABLE
HttpResponse DeviceManager::httpRequest(const char* method, const char* url, const char* payload, const char* contentType, const char* authToken) {
    HttpResponse resp;
    if (!url || !method || (!isWiFiConnected() && _state != DeviceState::CONFIGURING)) {
        _last_status_code = -1;
        return resp;
    }

    if (!_beginHttpUrl(url)) {
        _last_status_code = -1;
        return resp;
    }

    _applyCustomHeaders(authToken);
    if (payload && strlen(payload) > 0 && contentType && strlen(contentType) > 0) {
        _http.addHeader("Content-Type", contentType);
    }

    int code = -1;
    if (strcmp(method, "GET") == 0) {
        code = _http.GET();
    } else if (strcmp(method, "POST") == 0) {
        code = _http.POST((uint8_t*)(payload ? payload : ""), payload ? strlen(payload) : 0);
    } else if (strcmp(method, "PUT") == 0) {
        code = _http.PUT((uint8_t*)(payload ? payload : ""), payload ? strlen(payload) : 0);
    } else if (strcmp(method, "PATCH") == 0) {
        code = _http.PATCH((uint8_t*)(payload ? payload : ""), payload ? strlen(payload) : 0);
    } else {
        code = _http.sendRequest(method, (uint8_t*)(payload ? payload : ""), payload ? strlen(payload) : 0);
    }

    _last_status_code = code;
    _last_response = "";
    resp.statusCode = code;
    resp.contentLength = _http.getSize();
    if (code > 0) {
        _last_response = _http.getString();
        resp.body = _last_response;
    }
    _http.end();
    return resp;
}

String DeviceManager::httpGet(const char* url, const char* authToken) {
    HttpResponse r = httpRequest("GET", url, nullptr, nullptr, authToken);
    return r.ok() ? r.body : String("");
}

int DeviceManager::httpPost(const char* url, const char* payload, const char* contentType, const char* authToken) {
    HttpResponse r = httpRequest("POST", url, payload ? payload : "", contentType, authToken);
    return r.statusCode;
}

int DeviceManager::httpPut(const char* url, const char* payload, const char* contentType, const char* authToken) {
    HttpResponse r = httpRequest("PUT", url, payload ? payload : "", contentType, authToken);
    return r.statusCode;
}

int DeviceManager::httpPatch(const char* url, const char* payload, const char* contentType, const char* authToken) {
    HttpResponse r = httpRequest("PATCH", url, payload ? payload : "", contentType, authToken);
    return r.statusCode;
}

int DeviceManager::httpDelete(const char* url, const char* payload, const char* authToken) {
    HttpResponse r = httpRequest("DELETE", url, payload, "application/json", authToken);
    return r.statusCode;
}

int DeviceManager::httpPostFile(const char* url, uint8_t* data, size_t len, const char* fileName, const char* fileKey, const char* extraKey, const char* extraVal) {
    if (!url || !data || len == 0 || !isWiFiConnected()) return -1;

    String boundary = "----IntegrallBoundary" + String(millis());
    
    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"" + String(fileKey ? fileKey : "file") + "\"; filename=\"" + String(fileName ? fileName : "upload.bin") + "\"\r\n";
    head += "Content-Type: application/octet-stream\r\n\r\n";
    
    String tail = "\r\n--" + boundary;
    if (extraKey && extraVal) {
        tail += "\r\nContent-Disposition: form-data; name=\"" + String(extraKey) + "\"\r\n\r\n" + String(extraVal) + "\r\n--" + boundary;
    }
    tail += "--\r\n";
    
    size_t totalLen = head.length() + len + tail.length();
    uint8_t* buffer = nullptr;
    #if defined(ESP32)
    if (psramFound()) {
        buffer = (uint8_t*)ps_malloc(totalLen);
    }
    #endif
    if (!buffer) {
        buffer = (uint8_t*)malloc(totalLen);
    }
    if (!buffer) {
        return -2; // Out of memory
    }

    memcpy(buffer, head.c_str(), head.length());
    memcpy(buffer + head.length(), data, len);
    memcpy(buffer + head.length() + len, tail.c_str(), tail.length());

    if (!_beginHttpUrl(url)) {
        free(buffer);
        return -1;
    }

    _applyCustomHeaders();
    _http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    int code = _http.POST(buffer, totalLen);
    free(buffer);
    
    _last_status_code = code;
    _last_response = "";
    if (code > 0) _last_response = _http.getString();
    _http.end();
    return code;
}

Stream* DeviceManager::getStream(const char* url) {
    if (!url || !isWiFiConnected()) return nullptr;
    if (!_beginHttpUrl(url)) return nullptr;
    
    _applyCustomHeaders();
    int code = _http.GET();
    _last_status_code = code;
    if (code == 200) {
        #if defined(ESP32)
        return &_http.getStream();
        #else
        return &_wifiClient;
        #endif
    }
    _http.end();
    return nullptr;
}

void DeviceManager::endStream() {
    _http.end();
}
#endif

} // namespace Integrall
