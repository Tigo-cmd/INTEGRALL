/**
 * MqttModule.h
 *
 * Integrall cloud link over MQTT (INTEGRALL_ENABLE_MQTT).
 *
 * Push, never poll: the device keeps one MQTT session; the backend publishes
 * commands to it instantly and the device publishes telemetry/events.
 *
 * Non-blocking guarantees
 *  - ESP32: the MQTT client runs in its own FreeRTOS task (espMqttClient), so
 *    TCP/TLS connects and broker outages never stall loop().
 *  - ESP8266: plain MQTT uses the fully async client (ESPAsyncTCP). TLS on
 *    ESP8266 uses the sync client: a TLS (re)connect blocks for its duration.
 *  - Incoming commands are queued and dispatched from loop() (handle()), so
 *    command handlers may safely touch relays, LCD, I2C, etc.
 *
 * Topic contract (shared with cloud/backend — keep in sync with HANDOFF.md):
 *   {prefix}/{id}/status      retained  "online" | "offline" (LWT)
 *   {prefix}/{id}/meta        retained  {"fw","board","ip","mac","modules":[...]}
 *   {prefix}/{id}/telemetry   QoS0      {"ts":<epoch?>, "<key>": <value>, ...}
 *   {prefix}/{id}/event       QoS1      {"ts":<epoch?>, "type":"...", "detail":"..."}
 *   {prefix}/{id}/health      retained  {"parts":{"lcd":true,...}, "heap":..., "uptime":...}
 *   {prefix}/{id}/cmd         subscribe {"id":"<uuid>","action":"relay.set","args":{...}}
 *   {prefix}/{id}/cmd/ack     QoS1      {"id":"<uuid>","ok":true,"msg":"...","result":{...}}
 *
 * Auth: username = device id, password = device token (DeviceConfig::api_key).
 * Library: "espMqttClient" by Bert Melis (+ ESPAsyncTCP on ESP8266).
 */

#ifndef INTEGRALL_MQTT_MODULE_H
#define INTEGRALL_MQTT_MODULE_H

#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"

#if INTEGRALL_MODULE_MQTT_ENABLED && INTEGRALL_NETWORK_AVAILABLE

#include <ArduinoJson.h>
#include <espMqttClient.h>
#if defined(ESP8266)
  #include <espMqttClientAsync.h>
#endif

#ifndef INTEGRALL_MAX_CLOUD_SWITCHES
  #define INTEGRALL_MAX_CLOUD_SWITCHES 4   // addCloudSwitch() slots
#endif
#ifndef INTEGRALL_MQTT_MAX_CMD
  #define INTEGRALL_MQTT_MAX_CMD 512        // largest accepted command JSON
#endif
#ifndef INTEGRALL_MQTT_CMD_QUEUE
  #define INTEGRALL_MQTT_CMD_QUEUE 4
#endif
#ifndef INTEGRALL_MQTT_PREFIX
  #define INTEGRALL_MQTT_PREFIX "integrall"
#endif

namespace Integrall {

struct MqttConfig {
    const char* host = nullptr;
    uint16_t    port = 1883;          // 8883 for TLS
    bool        tls = false;
    const char* rootCA = nullptr;     // PEM; nullptr + tls = encrypted but unverified
    const char* username = nullptr;   // default: device id
    const char* password = nullptr;   // device token
    uint16_t    keepAliveSec = 30;
};

/** Result object a command handler fills in; sent back on .../cmd/ack. */
class CommandReply {
public:
    void ok(const char* msg = nullptr)   { _ok = true;  _msg = msg ? msg : ""; _set = true; }
    void fail(const char* msg = nullptr) { _ok = false; _msg = msg ? msg : ""; _set = true; }
    /** Extra structured data returned to the caller. */
    JsonObject result() { return _result.to<JsonObject>(); }
    bool handled() const { return _set; }
    bool isOk() const    { return _ok; }
    const String& message() const { return _msg; }
    const JsonDocument& resultDoc() const { return _result; }
private:
    bool _set = false, _ok = false;
    String _msg;
    JsonDocument _result;
};

/**
 * Command handler. Return after calling reply.ok()/fail(); leave reply
 * untouched to fall through to Integrall's built-in actions.
 */
typedef void (*CommandHandler)(const char* action, JsonVariantConst args, CommandReply& reply);

class MqttModule {
public:
    typedef void (*Hook)(void* ctx);
    typedef void (*BuiltinHandler)(void* ctx, const char* action, JsonVariantConst args, CommandReply& reply);

    MqttModule() {}

    /** Configure; connection happens in the background once WiFi is up. */
    bool begin(const MqttConfig& cfg, const char* deviceId) {
        if (!cfg.host || !cfg.host[0] || !deviceId) {
            INTEGRALL_LOG_ERROR("MQTT: host and device id required");
            return false;
        }
        _cfg = cfg;
        snprintf(_base, sizeof(_base), "%s/%s", INTEGRALL_MQTT_PREFIX, deviceId);
        snprintf(_t_status, sizeof(_t_status), "%s/status", _base);
        snprintf(_t_cmd, sizeof(_t_cmd), "%s/cmd", _base);
        strncpy(_client_id, deviceId, sizeof(_client_id) - 1);

        #if defined(ESP32)
        if (cfg.tls) {
            auto* c = new espMqttClientSecure(espMqttClientTypes::UseInternalTask::YES);
            if (cfg.rootCA) c->setCACert(cfg.rootCA); else c->setInsecure();
            _setup(*c);
            _client = c;
        } else {
            auto* c = new espMqttClient(espMqttClientTypes::UseInternalTask::YES);
            _setup(*c);
            _client = c;
        }
        #else
        if (cfg.tls) {
            auto* c = new espMqttClientSecure();
            if (cfg.rootCA) { _x509 = new BearSSL::X509List(cfg.rootCA); c->setTrustAnchors(_x509); }
            else c->setInsecure();
            c->setBufferSizes(1024, 1024);
            _setup(*c);
            _client = c;
        } else {
            auto* c = new espMqttClientAsync();
            _setup(*c);
            _client = c;
        }
        #endif
        _enabled = true;
        _next_try = 0;
        INTEGRALL_LOG_INFO_VAL("MQTT: broker ", cfg.host);
        return true;
    }

    bool isConnected() const { return _client && _client->connected(); }
    const char* baseTopic() const { return _base; }

    void onCommand(CommandHandler h) { _user_handler = h; }
    /** Internal: System hooks for connect (publish meta/health) and built-in commands. */
    void setHooks(Hook onConnected, BuiltinHandler builtins, void* ctx, Hook onReconnected = nullptr) {
        _hook_connected = onConnected; _builtin_handler = builtins; _ctx = ctx; _hook_reconnected = onReconnected;
    }

    /** Called by System::handle(). Never blocks (see ESP8266 TLS note above). */
    void handle(bool wifiUp) {
        if (!_enabled) return;
        #if defined(ESP8266)
        _client->loop();
        #endif
        const unsigned long now = millis();

        if (_evt_connected) {
            _evt_connected = false;
            _backoff = 2000;
            INTEGRALL_LOG_INFO("MQTT: connected");
            _client->subscribe(_t_cmd, 1);
            _client->publish(_t_status, 1, true, "online");
            if (_hook_reconnected) _hook_reconnected(_ctx);
            if (_hook_connected) _hook_connected(_ctx);
        }
        if (_evt_disconnected) {
            _evt_disconnected = false;
            INTEGRALL_LOG_WARN_VAL("MQTT: disconnected, reason ", (int)_disc_reason);
            _next_try = now + _backoff;
            _backoff = (_backoff >= 30000) ? 60000 : _backoff * 2;
        }

        if (wifiUp && _client->disconnected() && (long)(now - _next_try) >= 0) {
            _next_try = now + _backoff;              // in case this attempt silently dies
            _client->connect();
        }

        _drainCommands();
    }

    // ---- publishing (all return false if not connected) ---------------------
    bool publish(const char* suffix, const char* payload, uint8_t qos = 0, bool retain = false) {
        if (!isConnected() || !suffix || !payload) return false;
        char topic[96];
        snprintf(topic, sizeof(topic), "%s/%s", _base, suffix);
        return _client->publish(topic, qos, retain, payload) != 0;
    }

    bool publishJson(const char* suffix, const JsonDocument& doc, uint8_t qos = 0, bool retain = false) {
        if (!isConnected()) return false;
        String out;
        serializeJson(doc, out);
        return publish(suffix, out.c_str(), qos, retain);
    }

    bool publishTelemetry(const JsonDocument& data) {
        if (!isConnected()) return false;
        JsonDocument doc;
        _stamp(doc);
        if (data.is<JsonObjectConst>()) {
            for (JsonPairConst kv : data.as<JsonObjectConst>()) doc[kv.key()] = kv.value();
        } else {
            doc["value"] = data;
        }
        return publishJson("telemetry", doc, 0, false);
    }

    bool publishEvent(const char* type, const char* detail) {
        if (!isConnected()) return false;
        JsonDocument doc;
        _stamp(doc);
        doc["type"] = type;
        doc["detail"] = detail;
        return publishJson("event", doc, 1, false);
    }

private:
    MqttConfig     _cfg;
    MqttClient*    _client = nullptr;
    bool           _enabled = false;
    char           _base[64] = {0};
    char           _t_status[80] = {0};
    char           _t_cmd[80] = {0};
    char           _client_id[24] = {0};
    char           _will[8] = "offline";
    unsigned long  _next_try = 0;
    uint32_t       _backoff = 2000;
    #if defined(ESP8266)
    BearSSL::X509List* _x509 = nullptr;
    #endif

    // events from the MQTT task / async context
    volatile bool  _evt_connected = false;
    volatile bool  _evt_disconnected = false;
    volatile uint8_t _disc_reason = 0;

    // inbound command queue (producer: MQTT task, consumer: loop)
    struct InMsg { uint16_t len; char data[INTEGRALL_MQTT_MAX_CMD + 1]; };
    InMsg          _q[INTEGRALL_MQTT_CMD_QUEUE];
    volatile uint8_t _q_head = 0, _q_count = 0;
    char           _frag[INTEGRALL_MQTT_MAX_CMD + 1];
    size_t         _frag_len = 0;
    bool           _frag_drop = false;
    #if defined(ESP32)
    portMUX_TYPE   _mux = portMUX_INITIALIZER_UNLOCKED;
    void _lock()   { portENTER_CRITICAL(&_mux); }
    void _unlock() { portEXIT_CRITICAL(&_mux); }
    #else
    void _lock()   { noInterrupts(); }
    void _unlock() { interrupts(); }
    #endif

    CommandHandler _user_handler = nullptr;
    BuiltinHandler _builtin_handler = nullptr;
    Hook           _hook_connected = nullptr;
    Hook           _hook_reconnected = nullptr;   // runs before onConnected on every new session
    void*          _ctx = nullptr;

    template <typename T>
    void _setup(T& c) {
        c.setServer(_cfg.host, _cfg.port);
        c.setClientId(_client_id);
        c.setCredentials(_cfg.username ? _cfg.username : _client_id, _cfg.password);
        c.setKeepAlive(_cfg.keepAliveSec);
        c.setCleanSession(true);                       // never replay stale commands after an outage
        c.setWill(_t_status, 1, true, _will);
        c.onConnect([this](bool) { _evt_connected = true; });
        c.onDisconnect([this](espMqttClientTypes::DisconnectReason r) {
            _disc_reason = (uint8_t)r;
            _evt_disconnected = true;
        });
        c.onMessage([this](const espMqttClientTypes::MessageProperties&, const char* topic,
                           const uint8_t* payload, size_t len, size_t index, size_t total) {
            _onMessage(topic, payload, len, index, total);
        });
    }

    /** Runs in the MQTT task / async context: copy only, never act. */
    void _onMessage(const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total) {
        if (strcmp(topic, _t_cmd) != 0) return;
        if (index == 0) { _frag_len = 0; _frag_drop = (total > INTEGRALL_MQTT_MAX_CMD); }
        if (_frag_drop) return;
        if (_frag_len + len > INTEGRALL_MQTT_MAX_CMD) { _frag_drop = true; return; }
        memcpy(_frag + _frag_len, payload, len);
        _frag_len += len;
        if (index + len < total) return;              // more fragments coming
        _lock();
        if (_q_count < INTEGRALL_MQTT_CMD_QUEUE) {
            InMsg& m = _q[(_q_head + _q_count) % INTEGRALL_MQTT_CMD_QUEUE];
            memcpy(m.data, _frag, _frag_len);
            m.data[_frag_len] = '\0';
            m.len = (uint16_t)_frag_len;
            _q_count = _q_count + 1;
        }
        _unlock();
    }

    void _drainCommands() {
        while (true) {
            static char buf[INTEGRALL_MQTT_MAX_CMD + 1];
            _lock();
            if (_q_count == 0) { _unlock(); return; }
            InMsg& m = _q[_q_head];
            memcpy(buf, m.data, m.len + 1);
            _q_head = (_q_head + 1) % INTEGRALL_MQTT_CMD_QUEUE;
            _q_count = _q_count - 1;
            _unlock();
            _execute(buf);
        }
    }

    void _execute(const char* json) {
        JsonDocument in;
        JsonDocument ack;
        if (deserializeJson(in, json) != DeserializationError::Ok || !in["action"].is<const char*>()) {
            ack["id"] = in["id"] | "";
            ack["ok"] = false;
            ack["msg"] = "malformed command";
            publishJson("cmd/ack", ack, 1);
            return;
        }
        const char* action = in["action"];
        JsonVariantConst args = in["args"];
        CommandReply reply;
        if (_user_handler) _user_handler(action, args, reply);
        if (!reply.handled() && _builtin_handler) _builtin_handler(_ctx, action, args, reply);
        if (!reply.handled()) reply.fail("unknown action");

        ack["id"] = in["id"] | "";
        ack["ok"] = reply.isOk();
        if (reply.message().length()) ack["msg"] = reply.message();
        if (!reply.resultDoc().isNull()) ack["result"] = reply.resultDoc();
        publishJson("cmd/ack", ack, 1);
        INTEGRALL_LOG_INFO_VAL("MQTT: command ", action);
    }

    static void _stamp(JsonDocument& doc) {
        time_t t = time(nullptr);
        if (t > 1577836800) doc["ts"] = (uint32_t)t;     // only when NTP-synced; backend stamps otherwise
    }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_MQTT_ENABLED && INTEGRALL_NETWORK_AVAILABLE
#endif // INTEGRALL_MQTT_MODULE_H
