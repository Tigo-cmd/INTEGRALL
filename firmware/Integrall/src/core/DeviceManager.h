/**
 * DeviceManager.h
 *
 * Core device management for the Integrall Framework (header-only).
 *  - Non-blocking WiFi station / access-point management with exponential
 *    back-off reconnect and connection-change callbacks.
 *  - HTTP/HTTPS REST client (GET, POST, PUT, PATCH, DELETE, multipart upload).
 *  - Stable device identity derived from the MAC address.
 *
 * Networking is compiled ONLY when INTEGRALL_ENABLE_WIFI is defined (on ESP32 /
 * ESP8266). Without it, this class is a tiny offline stub and no WiFi/TLS code
 * is linked into the firmware.
 *
 * NOTE: The HTTP client calls are synchronous by nature (they wait for the
 * server, bounded by setHttpTimeout(), default 5 s). Use them for occasional
 * requests from your own code. Cloud telemetry/commands go through the
 * non-blocking MQTT module instead (INTEGRALL_ENABLE_MQTT).
 */

#ifndef INTEGRALL_DEVICE_MANAGER_H
#define INTEGRALL_DEVICE_MANAGER_H

#include "../config/IntegrallConfig.h"
#include "Logger.h"

#if INTEGRALL_NETWORK_AVAILABLE
  #include <ArduinoJson.h>
  #if ARDUINOJSON_VERSION_MAJOR < 7
    #error "Integrall networking requires ArduinoJson v7 or newer (Library Manager: ArduinoJson by Benoit Blanchon)."
  #endif
  #if defined(ESP32)
    #include <WiFi.h>
    #include <HTTPClient.h>
  #else
    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
  #endif
  #include <WiFiClient.h>
  #include <WiFiClientSecure.h>
#endif
#if defined(ESP8266)
  extern "C" {
    #include <user_interface.h>
  }
#endif

namespace Integrall {

// Connection states for the state machine
enum class DeviceState {
    UNINITIALIZED,      // Just created, begin() not called
    CONFIGURING,        // Running as an access point (beginAP / config portal)
    DISCONNECTED,       // WiFi configured but not connected (backing off)
    CONNECTING_WIFI,    // Association / DHCP in progress
    WIFI_CONNECTED,     // Kept for API compatibility (transitional, never sticky)
    REGISTERING,        // Kept for API compatibility (unused since v1.2)
    ONLINE,             // WiFi up (cloud link state: System::isCloudConnected())
    ERROR,              // Unrecoverable configuration error
    OFFLINE             // No networking configured / compiled
};

// Configuration structure
struct DeviceConfig {
    const char* wifi_ssid = nullptr;
    const char* wifi_password = nullptr;
    const char* backend_url = nullptr;      // "mqtt://host:1883" / "mqtts://host:8883" sets the MQTT broker;
                                            // any other URL is just stored for your own HTTP calls
    const char* api_key = nullptr;          // Device token (MQTT password)
    const char* mqtt_host = nullptr;        // Broker host (alternative to an mqtt:// backend_url)
    uint16_t    mqtt_port = 0;              // 0 = 1883, or 8883 with TLS
    bool        mqtt_tls = false;
    const char* mqtt_root_ca = nullptr;     // PEM root CA for TLS (nullptr = encrypted, not verified)
    uint16_t    backend_port = 8000;        // Deprecated, unused
    bool        use_wifi_manager = false;   // true = start as access point (beginAP)
    uint32_t    wifi_connect_timeout_ms = 15000; // Give up one attempt after this long
    uint32_t    reconnect_interval_ms = 2000;    // First back-off step (doubles up to 60 s)
    uint32_t    poll_interval_ms = 3000;    // Deprecated, unused (no polling since v1.2)
};

typedef void (*ConnectionCallback)(bool connected);

#if INTEGRALL_NETWORK_AVAILABLE
/** Structured HTTP/HTTPS response object */
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
    DeviceManager() { _device_id[0] = '\0'; _device_ip[0] = '\0'; _last_error[0] = '\0'; }

    /**
     * Start device management. Never blocks waiting for WiFi: the connection
     * completes in the background inside handle(). Use onConnectionChange()
     * or isWiFiConnected() to react when it is up.
     * @return false only on a configuration error.
     */
    bool begin(const DeviceConfig& config) {
        _config = config;
        Logger::begin();
        INTEGRALL_LOG_INFO("Integrall System Starting...");
        INTEGRALL_LOG_INFO_VAL("Version: ", INTEGRALL_VERSION_STRING);
        _generateDeviceId();
        INTEGRALL_LOG_INFO_VAL("Device ID: ", _device_id);

#if INTEGRALL_NETWORK_AVAILABLE
        if (_config.use_wifi_manager) {
            return startConfigPortal(_config.wifi_ssid ? _config.wifi_ssid : "Integrall-Setup",
                                     _config.wifi_password);
        }
        if (_config.wifi_ssid && _config.wifi_ssid[0] != '\0') {
            _wifi_configured = true;
            WiFi.persistent(false);       // don't wear flash on every begin
            WiFi.mode(WIFI_STA);
            #if defined(ESP32)
            WiFi.setHostname(_device_id);
            #else
            WiFi.hostname(_device_id);
            #endif
            WiFi.setAutoReconnect(true);
            _startAttempt();
            return true;
        }
        _setState(DeviceState::OFFLINE);
        return true;
#else
        #if defined(ESP32) || defined(ESP8266)
        if (config.wifi_ssid && config.wifi_ssid[0] != '\0') {
            INTEGRALL_LOG_WARN("WiFi credentials given but INTEGRALL_ENABLE_WIFI is not defined; staying offline");
        }
        #endif
        _setState(DeviceState::OFFLINE);
        return true;
#endif
    }

    /** Main loop tick. Returns immediately; never waits. */
    void handle() {
#if INTEGRALL_NETWORK_AVAILABLE
        if (!_wifi_configured || _state == DeviceState::CONFIGURING) return;

        const bool connected = (WiFi.status() == WL_CONNECTED);
        const unsigned long now = millis();

        if (connected) {
            if (_state != DeviceState::ONLINE) {
                _updateIpFromWiFi();
                _backoff_ms = _config.reconnect_interval_ms;
                INTEGRALL_LOG_INFO_VAL("WiFi connected, IP: ", _device_ip);
                _setState(DeviceState::ONLINE);
            }
            return;
        }

        // Not connected
        if (_state == DeviceState::ONLINE) {
            INTEGRALL_LOG_WARN("WiFi connection lost");
            _device_ip[0] = '\0';
            _setState(DeviceState::CONNECTING_WIFI);   // SDK auto-reconnect is already trying
            _attempt_started = now;
            return;
        }
        if (_state == DeviceState::CONNECTING_WIFI) {
            if (now - _attempt_started >= _config.wifi_connect_timeout_ms) {
                _setError("WiFi connect attempt timed out");
                INTEGRALL_LOG_WARN_VAL("WiFi attempt timed out; retrying in ms: ", _backoff_ms);
                _setState(DeviceState::DISCONNECTED);
                _next_attempt_at = now + _backoff_ms;
                _backoff_ms = (_backoff_ms >= 30000UL) ? 60000UL : _backoff_ms * 2;
            }
            return;
        }
        if (_state == DeviceState::DISCONNECTED && (long)(now - _next_attempt_at) >= 0) {
            _startAttempt();
        }
#endif
    }

    // ---- State queries -----------------------------------------------------
    DeviceState getState() const { return _state; }
    bool isOnline() const { return _state == DeviceState::ONLINE; }

    bool isWiFiConnected() const {
#if INTEGRALL_NETWORK_AVAILABLE
        return WiFi.status() == WL_CONNECTED;
#else
        return false;
#endif
    }

    const char* getDeviceId() const { return _device_id; }
    const char* getDeviceIp() const { return _device_ip; }

    int getWiFiStrength() const {
#if INTEGRALL_NETWORK_AVAILABLE
        return isWiFiConnected() ? WiFi.RSSI() : 0;
#else
        return 0;
#endif
    }

    /** Called (from loop context) whenever WiFi connects or disconnects. */
    void onConnectionChange(ConnectionCallback cb) { _conn_cb = cb; }

    const char* getLastError() const { return _last_error; }

    /** Force an immediate reconnect attempt (resets back-off). */
    void reconnect() {
#if INTEGRALL_NETWORK_AVAILABLE
        if (_wifi_configured) {
            _backoff_ms = _config.reconnect_interval_ms;
            _startAttempt();
        }
#endif
    }

    /** Start the board as an access point (SoftAP). Non-blocking. */
    bool startConfigPortal(const char* ap_name = "Integrall-Setup", const char* ap_password = nullptr) {
#if INTEGRALL_NETWORK_AVAILABLE
        if (ap_password && strlen(ap_password) > 0 && strlen(ap_password) < 8) {
            _setError("AP password must be at least 8 characters");
            INTEGRALL_LOG_ERROR("AP password must be at least 8 characters (WPA2)");
            _setState(DeviceState::ERROR);
            return false;
        }
        WiFi.mode(WIFI_AP);
        bool ok = WiFi.softAP(ap_name ? ap_name : "Integrall-Setup",
                              (ap_password && ap_password[0]) ? ap_password : nullptr);
        if (ok) {
            String ip = WiFi.softAPIP().toString();
            INTEGRALL_SAFE_STRCPY(_device_ip, ip.c_str(), sizeof(_device_ip));
            INTEGRALL_LOG_INFO_VAL("Access point started, IP: ", _device_ip);
            _setState(DeviceState::CONFIGURING);
        } else {
            _setError("softAP failed");
            _setState(DeviceState::ERROR);
        }
        return ok;
#else
        (void)ap_name; (void)ap_password;
        return false;
#endif
    }

    bool stopConfigPortal() {
#if INTEGRALL_NETWORK_AVAILABLE
        WiFi.softAPdisconnect(true);
        _setState(_wifi_configured ? DeviceState::DISCONNECTED : DeviceState::OFFLINE);
        return true;
#else
        return false;
#endif
    }

#if INTEGRALL_NETWORK_AVAILABLE
    // ---- HTTP/HTTPS client configuration ----------------------------------
    void setHttpHeader(const char* key, const char* value) {
        if (!key || !key[0]) return;
        for (uint8_t i = 0; i < _custom_header_count; i++) {
            if (_custom_headers[i].key.equalsIgnoreCase(key)) {
                _custom_headers[i].value = value ? value : "";
                return;
            }
        }
        if (_custom_header_count < MAX_CUSTOM_HEADERS) {
            _custom_headers[_custom_header_count].key = key;
            _custom_headers[_custom_header_count].value = value ? value : "";
            _custom_header_count++;
        } else {
            INTEGRALL_LOG_WARN("HTTP: custom header table full (max 6)");
        }
    }
    void clearHttpHeaders()                 { _custom_header_count = 0; }
    void setBearerToken(const char* token)  { _bearer_token = token ? token : ""; }
    /** Pin a root CA (PEM). Pass nullptr to go back to insecure mode. */
    void setCACert(const char* rootCA)      { _root_ca = rootCA; _tls_insecure = (rootCA == nullptr); }
    void setHttpInsecure(bool insecure = true) { _tls_insecure = insecure; }
    void setHttpTimeout(uint16_t timeoutMs) { _http_timeout_ms = timeoutMs > 0 ? timeoutMs : 5000; }

    // ---- HTTP/HTTPS requests (synchronous, bounded by setHttpTimeout) -----
    HttpResponse httpRequest(const char* method, const char* url, const char* payload = nullptr,
                             const char* contentType = "application/json", const char* authToken = nullptr) {
        HttpResponse resp;
        _last_status_code = -1;
        _last_response = "";
        if (!url || !method || !_networkUsable()) return resp;
        if (!_beginHttpUrl(url)) return resp;

        _applyCustomHeaders(authToken);
        const size_t plen = payload ? strlen(payload) : 0;
        if (plen > 0 && contentType && contentType[0]) {
            _http.addHeader("Content-Type", contentType);
        }
        const char* collect[] = {"Content-Type"};
        _http.collectHeaders(collect, 1);

        uint8_t* body = (uint8_t*)(payload ? payload : "");
        int code;
        if      (strcmp(method, "GET") == 0)   code = _http.GET();
        else if (strcmp(method, "POST") == 0)  code = _http.POST(body, plen);
        else if (strcmp(method, "PUT") == 0)   code = _http.PUT(body, plen);
        else if (strcmp(method, "PATCH") == 0) code = _http.PATCH(body, plen);
        else                                   code = _http.sendRequest(method, body, plen);

        _last_status_code = code;
        resp.statusCode = code;
        resp.contentLength = _http.getSize();
        if (code > 0) {
            resp.contentType = _http.header("Content-Type");
            _last_response = _http.getString();
            resp.body = _last_response;
        } else {
            _setError(HTTPClient::errorToString(code).c_str());
        }
        _http.end();
        return resp;
    }

    String httpGet(const char* url, const char* authToken = nullptr) {
        HttpResponse r = httpRequest("GET", url, nullptr, nullptr, authToken);
        return r.ok() ? r.body : String("");
    }
    int httpPost(const char* url, const char* payload, const char* contentType = "application/json", const char* authToken = nullptr) {
        return httpRequest("POST", url, payload ? payload : "", contentType, authToken).statusCode;
    }
    int httpPut(const char* url, const char* payload, const char* contentType = "application/json", const char* authToken = nullptr) {
        return httpRequest("PUT", url, payload ? payload : "", contentType, authToken).statusCode;
    }
    int httpPatch(const char* url, const char* payload, const char* contentType = "application/json", const char* authToken = nullptr) {
        return httpRequest("PATCH", url, payload ? payload : "", contentType, authToken).statusCode;
    }
    int httpDelete(const char* url, const char* payload = nullptr, const char* authToken = nullptr) {
        return httpRequest("DELETE", url, payload, "application/json", authToken).statusCode;
    }

    /**
     * POST a buffer as multipart/form-data. The body is streamed in three
     * parts (no full-size copy), so large camera frames fit in RAM.
     * @return HTTP status code, -1 on connection error.
     */
    int httpPostFile(const char* url, const uint8_t* data, size_t len, const char* fileName,
                     const char* fileKey = "file", const char* extraKey = nullptr, const char* extraVal = nullptr) {
        if (!url || !data || len == 0 || !_networkUsable()) return -1;
        char boundary[40];
        snprintf(boundary, sizeof(boundary), "----IntegrallBoundary%08lx", (unsigned long)micros());

        String head = String("--") + boundary + "\r\n";
        if (extraKey && extraVal) {
            head += String("Content-Disposition: form-data; name=\"") + extraKey + "\"\r\n\r\n" + extraVal + "\r\n--" + boundary + "\r\n";
        }
        head += String("Content-Disposition: form-data; name=\"") + (fileKey ? fileKey : "file") +
                "\"; filename=\"" + (fileName ? fileName : "upload.bin") + "\"\r\n";
        head += "Content-Type: application/octet-stream\r\n\r\n";
        String tail = String("\r\n--") + boundary + "--\r\n";

        MultipartStream stream(head, data, len, tail);
        if (!_beginHttpUrl(url)) return -1;
        _applyCustomHeaders();
        _http.addHeader("Content-Type", String("multipart/form-data; boundary=") + boundary);
        int code = _http.sendRequest("POST", &stream, stream.total());
        _last_status_code = code;
        _last_response = (code > 0) ? _http.getString() : String("");
        _http.end();
        return code;
    }

    /** Open a GET stream (e.g. audio). Call endStream() when done. */
    Stream* getStream(const char* url) {
        if (!url || !_networkUsable() || !_beginHttpUrl(url)) return nullptr;
        _applyCustomHeaders();
        int code = _http.GET();
        _last_status_code = code;
        if (code == 200) return _http.getStreamPtr();
        _http.end();
        return nullptr;
    }
    void endStream() { _http.end(); }

    const char* getLastResponse() const { return _last_response.c_str(); }
    int getLastStatusCode() const { return _last_status_code; }
#else
    const char* getLastResponse() const { return ""; }
    int getLastStatusCode() const { return -1; }
#endif

private:
    DeviceConfig  _config;
    DeviceState   _state = DeviceState::UNINITIALIZED;
    char          _device_id[20];   // "INT_" + 12 hex digits
    char          _device_ip[16];
    char          _last_error[40];
    bool          _wifi_configured = false;
    ConnectionCallback _conn_cb = nullptr;

#if INTEGRALL_NETWORK_AVAILABLE
    unsigned long _attempt_started = 0;
    unsigned long _next_attempt_at = 0;
    uint32_t      _backoff_ms = 2000;

    HTTPClient       _http;
    WiFiClient       _wifiClient;
    WiFiClientSecure _secureClient;
    static const uint8_t MAX_CUSTOM_HEADERS = 6;
    HttpHeaderKV _custom_headers[MAX_CUSTOM_HEADERS];
    uint8_t      _custom_header_count = 0;
    String       _bearer_token;
    const char*  _root_ca = nullptr;
    bool         _tls_insecure = true;
    uint16_t     _http_timeout_ms = 5000;
    String       _last_response;
    int          _last_status_code = 0;
    #if defined(ESP8266)
    BearSSL::X509List* _x509 = nullptr;
    const char*        _x509_src = nullptr;
    #endif

    /** Read-only Stream over (head, body, tail) used for multipart upload. */
    class MultipartStream : public Stream {
    public:
        MultipartStream(const String& h, const uint8_t* b, size_t bl, const String& t)
            : _h(h), _b(b), _bl(bl), _t(t) {}
        size_t total() const { return _h.length() + _bl + _t.length(); }
        int available() override { return (int)(total() - _pos); }
        int read() override {
            if (_pos >= total()) return -1;
            int c = _at(_pos); _pos++; return c;
        }
        int peek() override { return (_pos >= total()) ? -1 : _at(_pos); }
        size_t readBytes(char* buf, size_t n) override {
            size_t i = 0;
            while (i < n && _pos < total()) { buf[i++] = (char)_at(_pos++); }
            return i;
        }
        size_t write(uint8_t) override { return 0; }
        void flush() override {}
    private:
        int _at(size_t p) const {
            if (p < _h.length()) return (uint8_t)_h[p];
            p -= _h.length();
            if (p < _bl) return _b[p];
            p -= _bl;
            return (uint8_t)_t[p];
        }
        const String& _h; const uint8_t* _b; size_t _bl; const String& _t; size_t _pos = 0;
    };

    bool _networkUsable() const {
        return isWiFiConnected() || _state == DeviceState::CONFIGURING;
    }

    void _startAttempt() {
        INTEGRALL_LOG_INFO_VAL("Connecting to WiFi: ", _config.wifi_ssid);
        WiFi.disconnect();
        WiFi.begin(_config.wifi_ssid, _config.wifi_password);
        _attempt_started = millis();
        _setState(DeviceState::CONNECTING_WIFI);
    }

    void _updateIpFromWiFi() {
        String ip = WiFi.localIP().toString();
        INTEGRALL_SAFE_STRCPY(_device_ip, ip.c_str(), sizeof(_device_ip));
    }

    bool _beginHttpUrl(const char* url) {
        if (!url || !url[0]) return false;
        _http.setTimeout(_http_timeout_ms);
        #if defined(ESP32)
        _http.setConnectTimeout(_http_timeout_ms);
        #endif
        _http.setReuse(false);
        const bool https = (strncasecmp(url, "https://", 8) == 0);
        if (https) {
            #if defined(ESP32)
            if (_root_ca && !_tls_insecure) _secureClient.setCACert(_root_ca);
            else _secureClient.setInsecure();
            #else
            if (_root_ca && !_tls_insecure) {
                if (!_x509 || _x509_src != _root_ca) {
                    delete _x509;
                    _x509 = new BearSSL::X509List(_root_ca);
                    _x509_src = _root_ca;
                }
                _secureClient.setTrustAnchors(_x509);
            } else {
                _secureClient.setInsecure();
            }
            #endif
            return _http.begin(_secureClient, url);
        }
        return _http.begin(_wifiClient, url);
    }

    void _applyCustomHeaders(const char* authToken = nullptr) {
        for (uint8_t i = 0; i < _custom_header_count; i++) {
            if (_custom_headers[i].key.length() > 0) {
                _http.addHeader(_custom_headers[i].key, _custom_headers[i].value);
            }
        }
        const char* token = (authToken && authToken[0]) ? authToken
                          : (_bearer_token.length() > 0 ? _bearer_token.c_str() : nullptr);
        if (token) {
            if (strncmp(token, "Bearer ", 7) == 0 || strncmp(token, "Basic ", 6) == 0) {
                _http.addHeader("Authorization", token);
            } else {
                _http.addHeader("Authorization", String("Bearer ") + token);
            }
        }
    }
#endif

    void _generateDeviceId() {
#if defined(ESP32)
        uint64_t mac = ESP.getEfuseMac();
        snprintf(_device_id, sizeof(_device_id), "INT_%02X%02X%02X%02X%02X%02X",
                 (unsigned)(mac & 0xFF), (unsigned)((mac >> 8) & 0xFF), (unsigned)((mac >> 16) & 0xFF),
                 (unsigned)((mac >> 24) & 0xFF), (unsigned)((mac >> 32) & 0xFF), (unsigned)((mac >> 40) & 0xFF));
#elif defined(ESP8266)
        uint8_t mac[6] = {0};
        wifi_get_macaddr(STATION_IF, mac);   // SDK call: no need to link the WiFi library
        snprintf(_device_id, sizeof(_device_id), "INT_%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#else
        // AVR & others: no unique ID in hardware; stable but not unique.
        snprintf(_device_id, sizeof(_device_id), "INT_OFFLINE");
#endif
    }

    void _setState(DeviceState s) {
        if (s == _state) return;
        const bool was = (_state == DeviceState::ONLINE);
        _state = s;
        const bool now = (_state == DeviceState::ONLINE);
        if (was != now && _conn_cb) _conn_cb(now);
    }

    void _setError(const char* error) {
        if (error) INTEGRALL_SAFE_STRCPY(_last_error, error, sizeof(_last_error));
    }
};

} // namespace Integrall

#endif // INTEGRALL_DEVICE_MANAGER_H
