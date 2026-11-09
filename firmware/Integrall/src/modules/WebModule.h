/**
 * WebModule.h
 *
 * Complete Web, C++ HTML Tag & Dashboard Renderer, Native HTTP/REST WebServer
 * (GET, POST, PUT, PATCH, DELETE, CORS, Query/Form Args, JSON Body), and
 * Native Zero-Dependency RFC 6455 WebSockets (Server & Client) for Integrall.
 *
 * Built directly on WiFiServer/WiFiClient so it works on all ESP32 & ESP8266
 * cores without external libraries and without conflicting with esp_camera's
 * esp_http_server.h!
 */

#ifndef INTEGRALL_WEB_MODULE_H
#define INTEGRALL_WEB_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"

#if (defined(ESP32) || defined(ESP8266))
  #define INTEGRALL_WEBSERVER_AVAILABLE 1
  #if defined(ESP32)
    #include <WiFi.h>
  #elif defined(ESP8266)
    #include <ESP8266WiFi.h>
  #endif
#else
  #define INTEGRALL_WEBSERVER_AVAILABLE 0
#endif

namespace Integrall {

// ============================================================================
// 1. C++ HTML TAG & RESPONSIVE IOT DASHBOARD RENDERER (HtmlBuilder)
// ============================================================================
class HtmlBuilder {
public:
    HtmlBuilder() {}

    /**
     * Render any paired HTML tag: <tag attrs>innerHtml</tag>
     */
    static String tag(const char* tagName, const String& innerHtml = "", const char* attrs = nullptr) {
        if (!tagName || strlen(tagName) == 0) return innerHtml;
        String out = "<";
        out += tagName;
        if (attrs && strlen(attrs) > 0) {
            out += " ";
            out += attrs;
        }
        out += ">";
        out += innerHtml;
        out += "</";
        out += tagName;
        out += ">";
        return out;
    }

    /**
     * Render a void/self-closing HTML tag: <tag attrs>
     */
    static String voidTag(const char* tagName, const char* attrs = nullptr) {
        if (!tagName || strlen(tagName) == 0) return "";
        String out = "<";
        out += tagName;
        if (attrs && strlen(attrs) > 0) {
            out += " ";
            out += attrs;
        }
        out += ">";
        return out;
    }

    // --- Standard Semantic HTML5 Tags ---
    static String h1(const String& text, const char* attrs = nullptr)   { return tag("h1", text, attrs); }
    static String h2(const String& text, const char* attrs = nullptr)   { return tag("h2", text, attrs); }
    static String h3(const String& text, const char* attrs = nullptr)   { return tag("h3", text, attrs); }
    static String p(const String& text, const char* attrs = nullptr)    { return tag("p", text, attrs); }
    static String div(const String& inner, const char* attrs = nullptr) { return tag("div", inner, attrs); }
    static String span(const String& text, const char* attrs = nullptr) { return tag("span", text, attrs); }
    static String code(const String& text, const char* attrs = nullptr) { return tag("code", text, attrs); }
    static String pre(const String& text, const char* attrs = nullptr)  { return tag("pre", text, attrs); }
    static String ul(const String& items, const char* attrs = nullptr)  { return tag("ul", items, attrs); }
    static String li(const String& text, const char* attrs = nullptr)   { return tag("li", text, attrs); }
    static String br()                                                  { return "<br>"; }
    static String hr()                                                  { return "<hr>"; }

    static String a(const char* href, const String& text, const char* attrs = nullptr) {
        String aAttr = String("href=\"") + (href ? href : "#") + "\"";
        if (attrs && strlen(attrs) > 0) {
            aAttr += " ";
            aAttr += attrs;
        }
        return tag("a", text, aAttr.c_str());
    }

    static String img(const char* src, const char* alt = "", const char* attrs = nullptr) {
        String iAttr = String("src=\"") + (src ? src : "") + "\" alt=\"" + (alt ? alt : "") + "\"";
        if (attrs && strlen(attrs) > 0) {
            iAttr += " ";
            iAttr += attrs;
        }
        return voidTag("img", iAttr.c_str());
    }

    static String input(const char* type, const char* name, const String& value = "", const char* attrs = nullptr) {
        String inAttr = String("type=\"") + (type ? type : "text") + "\" name=\"" + (name ? name : "") + "\" value=\"" + value + "\"";
        if (attrs && strlen(attrs) > 0) {
            inAttr += " ";
            inAttr += attrs;
        }
        return voidTag("input", inAttr.c_str());
    }

    static String form(const char* action, const char* method, const String& innerHtml, const char* attrs = nullptr) {
        String fAttr = String("action=\"") + (action ? action : "/") + "\" method=\"" + (method ? method : "POST") + "\"";
        if (attrs && strlen(attrs) > 0) {
            fAttr += " ";
            fAttr += attrs;
        }
        return tag("form", innerHtml, fAttr.c_str());
    }

    // --- Interactive IoT Dashboard UI Components ---

    /**
     * Render a styled container card
     */
    static String card(const String& title, const String& bodyHtml) {
        String inner = "";
        if (title.length() > 0) {
            inner += "<div class=\"ig-card-title\">" + title + "</div>";
        }
        inner += "<div class=\"ig-card-body\">" + bodyHtml + "</div>";
        return tag("div", inner, "class=\"ig-card\"");
    }

    /**
     * Render a live sensor metric readout card (e.g., Temperature: 25.4 °C)
     */
    static String metric(const String& label, const String& value, const char* unit = "", const char* elementId = nullptr) {
        String valSpan = "<span class=\"ig-metric-val\"";
        if (elementId && strlen(elementId) > 0) {
            valSpan += String(" id=\"") + elementId + "\"";
        }
        valSpan += ">" + value + "</span>";
        if (unit && strlen(unit) > 0) {
            valSpan += String("<span class=\"ig-metric-unit\">") + unit + "</span>";
        }
        String inner = "<div class=\"ig-metric-lbl\">" + label + "</div><div class=\"ig-metric-row\">" + valSpan + "</div>";
        return tag("div", inner, "class=\"ig-card ig-metric\"");
    }

    /**
     * Render a status pill badge ("ONLINE", "ALERT", etc.)
     * @param color "green", "red", "cyan", or "amber"
     */
    static String badge(const String& text, const char* color = "cyan") {
        String cls = String("class=\"ig-badge ig-badge-") + (color ? color : "cyan") + "\"";
        return tag("span", text, cls.c_str());
    }

    /**
     * Render an interactive button that triggers an async fetch(endpoint) without page reload
     */
    static String button(const String& label, const char* endpoint, const char* color = "cyan") {
        String attr = String("class=\"ig-btn ig-btn-") + (color ? color : "cyan") + "\" onclick=\"igCmd('" + (endpoint ? endpoint : "/") + "')\"";
        return tag("button", label, attr.c_str());
    }

    /**
     * Render an interactive toggle switch that calls endpoint?state=1 or endpoint?state=0
     */
    static String toggle(const String& label, const char* id, bool checked, const char* endpoint) {
        String html = "<div class=\"ig-ctrl-row\"><span>" + label + "</span>";
        html += "<input type=\"checkbox\" id=\"" + String(id ? id : "sw") + "\"";
        if (checked) html += " checked";
        html += " onchange=\"igCmd('" + String(endpoint ? endpoint : "/toggle") + "?state='+(this.checked?1:0))\"/>";
        html += "</div>";
        return html;
    }

    /**
     * Render an interactive range slider that calls endpoint?val=X
     */
    static String slider(const String& label, const char* id, int minVal, int maxVal, int curVal, const char* endpoint) {
        String cid = id ? id : "rng";
        String html = "<div class=\"ig-ctrl-col\"><div class=\"ig-ctrl-row\"><span>" + label + "</span>";
        html += "<strong id=\"" + cid + "_v\">" + String(curVal) + "</strong></div>";
        html += "<input type=\"range\" class=\"ig-slider\" id=\"" + cid + "\" min=\"" + String(minVal) + "\" max=\"" + String(maxVal) + "\" value=\"" + String(curVal) + "\" ";
        html += "oninput=\"document.getElementById('" + cid + "_v').innerText=this.value\" ";
        html += "onchange=\"igCmd('" + String(endpoint ? endpoint : "/set") + "?val='+this.value)\"/></div>";
        return html;
    }

    /**
     * Render a progress bar (0 - 100%)
     */
    static String progressBar(const String& label, int percent, const char* elementId = nullptr) {
        int pct = constrain(percent, 0, 100);
        String html = "<div class=\"ig-ctrl-col\"><div class=\"ig-ctrl-row\"><span>" + label + "</span><span>" + String(pct) + "%</span></div>";
        html += "<div class=\"ig-bar-bg\"><div class=\"ig-bar-fill\"";
        if (elementId && strlen(elementId) > 0) html += String(" id=\"") + elementId + "\"";
        html += " style=\"width:" + String(pct) + "%\"></div></div></div>";
        return html;
    }

    /**
     * Inject a client-side WebSocket script that connects to ws://<host>:<wsPort>/
     * Automatically updates any DOM element matching JSON keys (e.g. {"temp":25.4} updates #temp)
     * or appends raw text to #targetId, and exposes window.wsSend(msg).
     */
    static String wsScript(uint16_t wsPort = 81, const char* targetId = "ws-log") {
        String s = "<script>(function(){var ws;function conn(){ws=new WebSocket('ws://'+location.hostname+':" + String(wsPort) + "/');";
        s += "ws.onmessage=function(e){try{var d=JSON.parse(e.data);for(var k in d){var el=document.getElementById(k);if(el)el.innerText=d[k];}}catch(_){";
        s += "var t=document.getElementById('" + String(targetId ? targetId : "ws-log") + "');if(t)t.innerText=e.data;}};";
        s += "ws.onclose=function(){setTimeout(conn,2000);};}conn();";
        s += "window.wsSend=function(m){if(ws&&ws.readyState===1)ws.send(typeof m==='object'?JSON.stringify(m):String(m));};})();</script>";
        return s;
    }

    /**
     * Wrap body HTML into a complete, responsive HTML5 document with built-in modern styling & fetch helper
     */
    static String page(const String& title, const String& bodyHtml, uint16_t autoRefreshSec = 0, bool darkTheme = true) {
        String doc = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">";
        doc += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
        if (autoRefreshSec > 0) {
            doc += "<meta http-equiv=\"refresh\" content=\"" + String(autoRefreshSec) + "\">";
        }
        doc += "<title>" + title + "</title><style>";
        if (darkTheme) {
            doc += ":root{--bg:#090b10;--card:#121621;--bdr:#22293a;--txt:#f1f5f9;--mut:#94a3b8;--acc:#00f2ff;--grn:#22c55e;--red:#ef4444;--amb:#f59e0b;}";
        } else {
            doc += ":root{--bg:#f8fafc;--card:#ffffff;--bdr:#e2e8f0;--txt:#0f172a;--mut:#64748b;--acc:#0284c7;--grn:#16a34a;--red:#dc2626;--amb:#d97706;}";
        }
        doc += "*{box-sizing:border-box;margin:0;padding:0;font-family:system-ui,-apple-system,sans-serif;}";
        doc += "body{background:var(--bg);color:var(--txt);padding:20px;max-width:920px;margin:0 auto;line-height:1.5;}";
        doc += "h1,h2,h3{margin-bottom:12px;letter-spacing:-0.02em;}";
        doc += ".ig-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:16px;margin:16px 0;}";
        doc += ".ig-card{background:var(--card);border:1px solid var(--bdr);border-radius:14px;padding:18px;margin-bottom:14px;}";
        doc += ".ig-card-title{font-size:12px;font-weight:700;text-transform:uppercase;letter-spacing:0.08em;color:var(--mut);margin-bottom:10px;}";
        doc += ".ig-metric-lbl{font-size:12px;text-transform:uppercase;letter-spacing:0.06em;color:var(--mut);}";
        doc += ".ig-metric-row{margin-top:6px;display:flex;align-items:baseline;gap:6px;}";
        doc += ".ig-metric-val{font-size:28px;font-weight:800;color:var(--acc);}";
        doc += ".ig-metric-unit{font-size:14px;color:var(--mut);}";
        doc += ".ig-btn{cursor:pointer;border:none;border-radius:10px;padding:10px 18px;font-weight:700;font-size:14px;margin:4px 4px 4px 0;transition:opacity .15s;}";
        doc += ".ig-btn:active{opacity:.7;}.ig-btn-cyan{background:var(--acc);color:#000;}.ig-btn-green{background:var(--grn);color:#fff;}.ig-btn-red{background:var(--red);color:#fff;}.ig-btn-amber{background:var(--amb);color:#000;}";
        doc += ".ig-badge{display:inline-block;padding:3px 10px;border-radius:99px;font-size:11px;font-weight:700;text-transform:uppercase;}";
        doc += ".ig-badge-cyan{background:rgba(0,242,255,.15);color:var(--acc);}.ig-badge-green{background:rgba(34,197,94,.18);color:var(--grn);}.ig-badge-red{background:rgba(239,68,68,.18);color:var(--red);}.ig-badge-amber{background:rgba(245,158,11,.18);color:var(--amb);}";
        doc += ".ig-ctrl-row{display:flex;justify-content:space-between;align-items:center;padding:8px 0;}";
        doc += ".ig-ctrl-col{padding:8px 0;}.ig-slider{width:100%;margin-top:6px;accent-color:var(--acc);}";
        doc += ".ig-bar-bg{width:100%;height:10px;background:var(--bdr);border-radius:99px;overflow:hidden;margin-top:6px;}";
        doc += ".ig-bar-fill{height:100%;background:var(--acc);transition:width .25s;}";
        doc += "</style></head><body>";
        doc += bodyHtml;
        doc += "<script>function igCmd(u){fetch(u).catch(function(e){console.error(e);});}</script></body></html>";
        return doc;
    }

    // --- Instance Fluent Builder Methods ---
    HtmlBuilder& add(const String& rawHtml) { _buf += rawHtml; return *this; }
    HtmlBuilder& addTag(const char* tagName, const String& inner, const char* attrs = nullptr) { _buf += tag(tagName, inner, attrs); return *this; }
    HtmlBuilder& addH1(const String& text)  { _buf += h1(text); return *this; }
    HtmlBuilder& addH2(const String& text)  { _buf += h2(text); return *this; }
    HtmlBuilder& addP(const String& text)   { _buf += p(text); return *this; }
    HtmlBuilder& addCard(const String& title, const String& body) { _buf += card(title, body); return *this; }
    HtmlBuilder& addMetric(const String& label, const String& value, const char* unit = "", const char* id = nullptr) { _buf += metric(label, value, unit, id); return *this; }
    HtmlBuilder& addButton(const String& label, const char* endpoint, const char* color = "cyan") { _buf += button(label, endpoint, color); return *this; }
    HtmlBuilder& addToggle(const String& label, const char* id, bool checked, const char* endpoint) { _buf += toggle(label, id, checked, endpoint); return *this; }
    HtmlBuilder& addSlider(const String& label, const char* id, int minV, int maxV, int curV, const char* endpoint) { _buf += slider(label, id, minV, maxV, curV, endpoint); return *this; }
    HtmlBuilder& addProgressBar(const String& label, int pct, const char* id = nullptr) { _buf += progressBar(label, pct, id); return *this; }
    HtmlBuilder& addWebSocket(uint16_t wsPort = 81, const char* targetId = "ws-log") { _buf += wsScript(wsPort, targetId); return *this; }
    void clear() { _buf = ""; }
    const String& str() const { return _buf; }
    String buildPage(const String& title, uint16_t autoRefreshSec = 0, bool darkTheme = true) const {
        return page(title, _buf, autoRefreshSec, darkTheme);
    }

private:
    String _buf;
};

// ============================================================================
// 2. PURE C++ SHA-1 + BASE64 FOR RFC 6455 WEBSOCKET HANDSHAKE (ZERO DEPS)
// ============================================================================
namespace Detail {

inline uint32_t rol32(uint32_t value, unsigned int bits) {
    return (value << bits) | (value >> (32 - bits));
}

inline void sha1Raw(const uint8_t* data, size_t len, uint8_t digest[20]) {
    uint32_t h0 = 0x67452301UL;
    uint32_t h1 = 0xEFCDAB89UL;
    uint32_t h2 = 0x98BADCFEUL;
    uint32_t h3 = 0x10325476UL;
    uint32_t h4 = 0xC3D2E1F0UL;

    uint64_t totalBits = (uint64_t)len * 8ULL;
    size_t paddedLen = ((len + 8) / 64 + 1) * 64;
    uint8_t block[64];

    for (size_t offset = 0; offset < paddedLen; offset += 64) {
        memset(block, 0, sizeof(block));
        for (size_t i = 0; i < 64; i++) {
            size_t idx = offset + i;
            if (idx < len) {
                block[i] = data[idx];
            } else if (idx == len) {
                block[i] = 0x80;
            }
        }
        if (offset + 64 == paddedLen) {
            for (int i = 0; i < 8; i++) {
                block[63 - i] = (uint8_t)((totalBits >> (i * 8)) & 0xFF);
            }
        }

        uint32_t w[80];
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)block[i * 4] << 24) |
                   ((uint32_t)block[i * 4 + 1] << 16) |
                   ((uint32_t)block[i * 4 + 2] << 8) |
                   ((uint32_t)block[i * 4 + 3]);
        }
        for (int i = 16; i < 80; i++) {
            w[i] = rol32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999UL;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1UL;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDCUL;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6UL;
            }
            uint32_t temp = rol32(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rol32(b, 30);
            b = a;
            a = temp;
        }
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }

    uint32_t hs[5] = { h0, h1, h2, h3, h4 };
    for (int i = 0; i < 5; i++) {
        digest[i * 4]     = (uint8_t)((hs[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = (uint8_t)((hs[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = (uint8_t)((hs[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = (uint8_t)(hs[i] & 0xFF);
    }
}

inline String base64Encode(const uint8_t* data, size_t len) {
    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String out;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = ((uint32_t)data[i]) << 16;
        if (i + 1 < len) n |= ((uint32_t)data[i + 1]) << 8;
        if (i + 2 < len) n |= ((uint32_t)data[i + 2]);
        out += tbl[(n >> 18) & 0x3F];
        out += tbl[(n >> 12) & 0x3F];
        out += (i + 1 < len) ? tbl[(n >> 6) & 0x3F] : '=';
        out += (i + 2 < len) ? tbl[n & 0x3F] : '=';
    }
    return out;
}

inline String computeWsAcceptKey(const String& clientKey) {
    String combined = clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    uint8_t hash[20];
    sha1Raw((const uint8_t*)combined.c_str(), combined.length(), hash);
    return base64Encode(hash, 20);
}

} // namespace Detail

// ============================================================================
// 3. LOCAL HTTP/REST WEB SERVER + RFC 6455 WEBSOCKETS ENGINE (WebModule)
// ============================================================================
typedef void (*WebRouteCallback)();
typedef void (*WsMessageCallback)(const char* message);

class WebModule {
public:
    WebModule()
        : _server_running(false),
          _ws_server_running(false),
          _ws_client_connected(false),
          _ws_msg_cb(nullptr),
          _ws_client_msg_cb(nullptr) {
        #if INTEGRALL_WEBSERVER_AVAILABLE
        _http_server = nullptr;
        _ws_server   = nullptr;
        _route_count = 0;
        _arg_count   = 0;
        _responded   = false;
        for (uint8_t i = 0; i < MAX_WS_CLIENTS; i++) {
            _ws_handshake_done[i] = false;
        }
        #endif
    }

    ~WebModule() {
        #if INTEGRALL_WEBSERVER_AVAILABLE
        if (_http_server) {
            delete _http_server;
            _http_server = nullptr;
        }
        if (_ws_server) {
            delete _ws_server;
            _ws_server = nullptr;
        }
        #endif
    }

#if INTEGRALL_WEBSERVER_AVAILABLE
    /**
     * Start the local HTTP/REST Web Server on the specified port (default: 80)
     */
    bool beginServer(uint16_t port = 80) {
        if (_server_running && _http_server) return true;
        _http_server = new WiFiServer(port);
        if (!_http_server) return false;
        _http_server->begin();
        _server_running = true;
        INTEGRALL_LOG_INFO_VAL("WebModule: HTTP Server started on port ", port);
        return true;
    }

    /**
     * Register GET / POST / PUT / PATCH / DELETE route handlers
     */
    void onGet(const char* uri, WebRouteCallback handler)    { _addRoute("GET", uri, handler, "", nullptr); }
    void onPost(const char* uri, WebRouteCallback handler)   { _addRoute("POST", uri, handler, "", nullptr); }
    void onPut(const char* uri, WebRouteCallback handler)    { _addRoute("PUT", uri, handler, "", nullptr); }
    void onPatch(const char* uri, WebRouteCallback handler)  { _addRoute("PATCH", uri, handler, "", nullptr); }
    void onDelete(const char* uri, WebRouteCallback handler) { _addRoute("DELETE", uri, handler, "", nullptr); }

    /**
     * Serve an HTML page at a given URI in one line (e.g., servePage("/", html))
     */
    void servePage(const char* uri, const String& html) {
        _addRoute("GET", uri, nullptr, html, "text/html; charset=utf-8");
    }

    /**
     * Serve a static/cached JSON payload at a given URI
     */
    void serveJson(const char* uri, const String& json) {
        _addRoute("GET", uri, nullptr, json, "application/json");
    }

    /**
     * Read query parameter (?key=val) or form argument inside a route handler
     */
    String arg(const char* name) const {
        if (!name) return "";
        for (uint8_t i = 0; i < _arg_count; i++) {
            if (_arg_keys[i].equalsIgnoreCase(name)) {
                return _arg_vals[i];
            }
        }
        return "";
    }

    /**
     * Read raw request body (e.g. JSON POST/PUT body) inside a route handler
     */
    const String& body() const {
        return _req_body;
    }

    /**
     * Send HTTP responses with automatic CORS headers
     */
    void sendHtml(const String& html, int statusCode = 200) {
        _sendHttpResponse(statusCode, "text/html; charset=utf-8", html);
    }

    void sendJson(const String& json, int statusCode = 200) {
        _sendHttpResponse(statusCode, "application/json", json);
    }

    void sendText(const String& text, int statusCode = 200) {
        _sendHttpResponse(statusCode, "text/plain; charset=utf-8", text);
    }

    void sendRedirect(const char* location, int statusCode = 302) {
        if (!_active_client || !_active_client.connected() || !location) return;
        String hdr = "HTTP/1.1 " + String(statusCode) + " Found\r\n";
        hdr += "Location: " + String(location) + "\r\n";
        hdr += "Access-Control-Allow-Origin: *\r\n";
        hdr += "Content-Length: 0\r\nConnection: close\r\n\r\n";
        _active_client.print(hdr);
        _responded = true;
    }

    // ========================================================================
    // RFC 6455 WEBSOCKET SERVER
    // ========================================================================

    /**
     * Start a real-time RFC 6455 WebSocket Server (default port: 81)
     */
    bool beginWsServer(uint16_t port = 81) {
        if (_ws_server_running && _ws_server) return true;
        _ws_server = new WiFiServer(port);
        if (!_ws_server) return false;
        _ws_server->begin();
        _ws_server_running = true;
        INTEGRALL_LOG_INFO_VAL("WebModule: WebSocket Server started on port ", port);
        return true;
    }

    /**
     * Register a callback invoked whenever a WebSocket client sends a text message
     */
    void onWsMessage(WsMessageCallback cb) {
        _ws_msg_cb = cb;
    }

    /**
     * Broadcast a text or JSON message to all connected WebSocket clients
     */
    uint8_t wsBroadcast(const char* message) {
        if (!_ws_server_running || !message) return 0;
        uint8_t sent = 0;
        size_t len = strlen(message);
        for (uint8_t i = 0; i < MAX_WS_CLIENTS; i++) {
            if (_ws_clients[i] && _ws_clients[i].connected() && _ws_handshake_done[i]) {
                if (_sendWsFrame(_ws_clients[i], 0x01, (const uint8_t*)message, len, false)) {
                    sent++;
                }
            }
        }
        return sent;
    }

    /**
     * Return the number of currently active WebSocket clients
     */
    uint8_t wsClientCount() {
        uint8_t count = 0;
        for (uint8_t i = 0; i < MAX_WS_CLIENTS; i++) {
            if (_ws_clients[i] && _ws_clients[i].connected() && _ws_handshake_done[i]) {
                count++;
            }
        }
        return count;
    }

    // ========================================================================
    // RFC 6455 WEBSOCKET CLIENT
    // ========================================================================

    /**
     * Connect to a remote WebSocket server via ws://host:port/path
     */
    bool wsConnect(const char* url) {
        if (!url) return false;
        const char* p = url;
        if (strncmp(p, "ws://", 5) == 0) p += 5;
        else if (strncmp(p, "wss://", 6) == 0) p += 6;

        String hostPortPath = String(p);
        int slashIdx = hostPortPath.indexOf('/');
        String hostPort = (slashIdx >= 0) ? hostPortPath.substring(0, slashIdx) : hostPortPath;
        String path     = (slashIdx >= 0) ? hostPortPath.substring(slashIdx) : "/";

        uint16_t port = 80;
        int colonIdx = hostPort.indexOf(':');
        String host = hostPort;
        if (colonIdx >= 0) {
            host = hostPort.substring(0, colonIdx);
            port = (uint16_t)hostPort.substring(colonIdx + 1).toInt();
        }
        return wsConnect(host.c_str(), port, path.c_str());
    }

    /**
     * Connect to a remote WebSocket server by host, port, and path
     */
    bool wsConnect(const char* host, uint16_t port, const char* path = "/") {
        if (!host || strlen(host) == 0) return false;
        _ws_client.stop();
        _ws_client_connected = false;

        if (!_ws_client.connect(host, port)) {
            INTEGRALL_LOG_ERROR("WebModule: WebSocket client TCP connect failed");
            return false;
        }

        // Send RFC 6455 Upgrade request
        String req = String("GET ") + (path ? path : "/") + " HTTP/1.1\r\n";
        req += String("Host: ") + host + ":" + String(port) + "\r\n";
        req += "Upgrade: websocket\r\n";
        req += "Connection: Upgrade\r\n";
        req += "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n";
        req += "Sec-WebSocket-Version: 13\r\n\r\n";
        _ws_client.print(req);

        unsigned long start = millis();
        while (!_ws_client.available() && (millis() - start < 3000)) {
            delay(10);
        }
        if (!_ws_client.available()) {
            _ws_client.stop();
            return false;
        }

        String statusLine = _ws_client.readStringUntil('\n');
        if (statusLine.indexOf("101") < 0) {
            _ws_client.stop();
            return false;
        }
        // Drain response headers
        while (_ws_client.connected() && _ws_client.available()) {
            String line = _ws_client.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) break;
        }

        _ws_client_connected = true;
        INTEGRALL_LOG_INFO_VAL("WebModule: WebSocket client connected to ", host);
        return true;
    }

    /**
     * Send a text or JSON message from the WebSocket client to the remote server
     */
    bool wsSend(const char* message) {
        if (!_ws_client_connected || !_ws_client.connected() || !message) return false;
        return _sendWsFrame(_ws_client, 0x01, (const uint8_t*)message, strlen(message), true);
    }

    /**
     * Register a callback for incoming messages on the WebSocket client
     */
    void onWsClientMessage(WsMessageCallback cb) {
        _ws_client_msg_cb = cb;
    }

    bool wsConnected() {
        return _ws_client_connected && _ws_client.connected();
    }

    void wsDisconnect() {
        if (_ws_client.connected()) {
            _sendWsFrame(_ws_client, 0x08, nullptr, 0, true);
            _ws_client.stop();
        }
        _ws_client_connected = false;
    }
#endif // INTEGRALL_WEBSERVER_AVAILABLE

    /**
     * Non-blocking pump called automatically inside Integrall::System::handle()
     */
    void handle() {
        #if INTEGRALL_WEBSERVER_AVAILABLE
        if (_server_running && _http_server) {
            _handleHttpServer();
        }
        if (_ws_server_running && _ws_server) {
            _handleWsServer();
        }
        if (_ws_client_connected) {
            if (!_ws_client.connected()) {
                _ws_client_connected = false;
            } else if (_ws_client.available()) {
                _readWsFrame(_ws_client, _ws_client_msg_cb, true);
            }
        }
        #endif
    }

private:
    bool _server_running;
    bool _ws_server_running;
    bool _ws_client_connected;
    WsMessageCallback _ws_msg_cb;
    WsMessageCallback _ws_client_msg_cb;

#if INTEGRALL_WEBSERVER_AVAILABLE
    struct RouteEntry {
        String           method;
        String           uri;
        WebRouteCallback handler;
        String           staticPayload;
        const char*      staticMime;
    };

    static const uint8_t MAX_WEB_ROUTES = 16;
    static const uint8_t MAX_QUERY_ARGS = 8;
    static const uint8_t MAX_WS_CLIENTS = 4;

    WiFiServer* _http_server;
    WiFiClient  _active_client;
    RouteEntry  _routes[MAX_WEB_ROUTES];
    uint8_t     _route_count;
    String      _arg_keys[MAX_QUERY_ARGS];
    String      _arg_vals[MAX_QUERY_ARGS];
    uint8_t     _arg_count;
    String      _req_body;
    bool        _responded;

    WiFiServer* _ws_server;
    WiFiClient  _ws_clients[MAX_WS_CLIENTS];
    bool        _ws_handshake_done[MAX_WS_CLIENTS];
    WiFiClient  _ws_client;

    void _addRoute(const char* method, const char* uri, WebRouteCallback handler, const String& payload, const char* mime) {
        if (!uri) return;
        if (!_http_server) beginServer(80);
        // Update existing route if method + uri already registered
        for (uint8_t i = 0; i < _route_count; i++) {
            if (_routes[i].method == method && _routes[i].uri == uri) {
                _routes[i].handler       = handler;
                _routes[i].staticPayload = payload;
                _routes[i].staticMime    = mime;
                return;
            }
        }
        if (_route_count < MAX_WEB_ROUTES) {
            _routes[_route_count].method        = method;
            _routes[_route_count].uri           = uri;
            _routes[_route_count].handler       = handler;
            _routes[_route_count].staticPayload = payload;
            _routes[_route_count].staticMime    = mime;
            _route_count++;
        }
    }

    void _parseQueryParams(const String& query) {
        int start = 0;
        while (start < (int)query.length() && _arg_count < MAX_QUERY_ARGS) {
            int amp = query.indexOf('&', start);
            String pair = (amp >= 0) ? query.substring(start, amp) : query.substring(start);
            int eq = pair.indexOf('=');
            if (eq > 0) {
                _arg_keys[_arg_count] = pair.substring(0, eq);
                _arg_vals[_arg_count] = pair.substring(eq + 1);
                _arg_count++;
            }
            if (amp < 0) break;
            start = amp + 1;
        }
    }

    void _sendHttpResponse(int statusCode, const char* contentType, const String& bodyContent) {
        if (!_active_client || !_active_client.connected()) return;
        const char* statusText = (statusCode == 200) ? "OK" :
                                 (statusCode == 201) ? "Created" :
                                 (statusCode == 204) ? "No Content" :
                                 (statusCode == 400) ? "Bad Request" :
                                 (statusCode == 404) ? "Not Found" : "OK";
        String hdr = "HTTP/1.1 " + String(statusCode) + " " + statusText + "\r\n";
        hdr += "Content-Type: " + String(contentType ? contentType : "text/plain") + "\r\n";
        hdr += "Access-Control-Allow-Origin: *\r\n";
        hdr += "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n";
        hdr += "Access-Control-Allow-Headers: *\r\n";
        hdr += "Content-Length: " + String(bodyContent.length()) + "\r\n";
        hdr += "Connection: close\r\n\r\n";
        _active_client.print(hdr);
        if (bodyContent.length() > 0) {
            _active_client.print(bodyContent);
        }
        _responded = true;
    }

    void _handleHttpServer() {
        #if defined(ESP32) && defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        WiFiClient client = _http_server->accept();
        #else
        WiFiClient client = _http_server->available();
        #endif
        if (!client) return;

        unsigned long start = millis();
        while (client.connected() && !client.available() && (millis() - start < 250)) {
            delay(1);
        }
        if (!client.available()) {
            client.stop();
            return;
        }

        // Parse request line: METHOD /uri?query HTTP/1.1
        String reqLine = client.readStringUntil('\n');
        reqLine.trim();
        int sp1 = reqLine.indexOf(' ');
        int sp2 = reqLine.indexOf(' ', sp1 + 1);
        if (sp1 < 0 || sp2 < 0) {
            client.stop();
            return;
        }
        String method  = reqLine.substring(0, sp1);
        String fullUri = reqLine.substring(sp1 + 1, sp2);
        String path    = fullUri;

        _arg_count = 0;
        _req_body  = "";
        int qIdx = fullUri.indexOf('?');
        if (qIdx >= 0) {
            path = fullUri.substring(0, qIdx);
            _parseQueryParams(fullUri.substring(qIdx + 1));
        }

        // Parse HTTP headers
        int contentLen = 0;
        bool isFormEncoded = false;
        while (client.connected()) {
            String hLine = client.readStringUntil('\n');
            hLine.trim();
            if (hLine.length() == 0) break;
            if (hLine.startsWith("Content-Length:") || hLine.startsWith("content-length:")) {
                contentLen = hLine.substring(15).toInt();
            } else if (hLine.indexOf("application/x-www-form-urlencoded") >= 0) {
                isFormEncoded = true;
            }
        }

        // Read request body if present
        if (contentLen > 0) {
            if (contentLen > 2048) contentLen = 2048;
            _req_body.reserve(contentLen);
            unsigned long bStart = millis();
            while ((int)_req_body.length() < contentLen && (millis() - bStart < 500)) {
                if (client.available()) {
                    _req_body += (char)client.read();
                } else {
                    delay(1);
                }
            }
            if (isFormEncoded) {
                _parseQueryParams(_req_body);
            }
        }

        _active_client = client;
        _responded = false;

        // Handle CORS Preflight
        if (method == "OPTIONS") {
            _sendHttpResponse(204, "text/plain", "");
            _active_client.stop();
            return;
        }

        // Match route
        bool matched = false;
        for (uint8_t i = 0; i < _route_count; i++) {
            if (_routes[i].uri == path && (_routes[i].method == method || _routes[i].method == "ANY")) {
                matched = true;
                if (_routes[i].handler) {
                    _routes[i].handler();
                } else if (_routes[i].staticMime) {
                    _sendHttpResponse(200, _routes[i].staticMime, _routes[i].staticPayload);
                }
                break;
            }
        }

        if (!matched) {
            _sendHttpResponse(404, "application/json", "{\"error\":\"Not Found\"}");
        } else if (!_responded) {
            _sendHttpResponse(200, "text/plain", "OK");
        }

        _active_client.stop();
    }

    void _handleWsServer() {
        #if defined(ESP32) && defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        WiFiClient incoming = _ws_server->accept();
        if (incoming) {
            bool accepted = false;
            for (uint8_t i = 0; i < MAX_WS_CLIENTS; i++) {
                if (!_ws_clients[i] || !_ws_clients[i].connected()) {
                    _ws_clients[i] = incoming;
                    _ws_handshake_done[i] = false;
                    accepted = true;
                    break;
                }
            }
            if (!accepted) {
                incoming.stop();
            }
        }
        #else
        if (_ws_server->hasClient()) {
            bool accepted = false;
            for (uint8_t i = 0; i < MAX_WS_CLIENTS; i++) {
                if (!_ws_clients[i] || !_ws_clients[i].connected()) {
                    _ws_clients[i] = _ws_server->available();
                    _ws_handshake_done[i] = false;
                    accepted = true;
                    break;
                }
            }
            if (!accepted) {
                WiFiClient rejectClient = _ws_server->available();
                rejectClient.stop();
            }
        }
        #endif

        for (uint8_t i = 0; i < MAX_WS_CLIENTS; i++) {
            if (_ws_clients[i] && _ws_clients[i].connected()) {
                if (!_ws_handshake_done[i]) {
                    if (_ws_clients[i].available()) {
                        _performServerHandshake(i);
                    }
                } else if (_ws_clients[i].available()) {
                    if (!_readWsFrame(_ws_clients[i], _ws_msg_cb, false)) {
                        _ws_clients[i].stop();
                        _ws_handshake_done[i] = false;
                    }
                }
            } else if (_ws_handshake_done[i]) {
                _ws_handshake_done[i] = false;
            }
        }
    }

    void _performServerHandshake(uint8_t idx) {
        WiFiClient& client = _ws_clients[idx];
        String wsKey = "";
        unsigned long timeout = millis();
        while (client.connected() && (millis() - timeout < 1000)) {
            if (!client.available()) {
                delay(1);
                continue;
            }
            String line = client.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) break;
            if (line.startsWith("Sec-WebSocket-Key:") || line.startsWith("sec-websocket-key:")) {
                wsKey = line.substring(18);
                wsKey.trim();
            }
        }
        if (wsKey.length() == 0) {
            client.stop();
            return;
        }
        String acceptKey = Detail::computeWsAcceptKey(wsKey);
        String resp = "HTTP/1.1 101 Switching Protocols\r\n";
        resp += "Upgrade: websocket\r\n";
        resp += "Connection: Upgrade\r\n";
        resp += "Sec-WebSocket-Accept: " + acceptKey + "\r\n\r\n";
        client.print(resp);
        _ws_handshake_done[idx] = true;
    }

    bool _sendWsFrame(WiFiClient& client, uint8_t opcode, const uint8_t* payload, size_t len, bool maskFrame) {
        if (!client.connected()) return false;
        uint8_t hdr[14];
        size_t hdrLen = 0;
        hdr[0] = 0x80 | (opcode & 0x0F); // FIN + opcode

        uint8_t maskBit = maskFrame ? 0x80 : 0x00;
        if (len <= 125) {
            hdr[1] = maskBit | (uint8_t)len;
            hdrLen = 2;
        } else if (len <= 65535) {
            hdr[1] = maskBit | 126;
            hdr[2] = (uint8_t)((len >> 8) & 0xFF);
            hdr[3] = (uint8_t)(len & 0xFF);
            hdrLen = 4;
        } else {
            return false;
        }

        uint8_t maskKey[4] = { 0x37, 0xFA, 0x21, 0x3D };
        if (maskFrame) {
            hdr[hdrLen++] = maskKey[0];
            hdr[hdrLen++] = maskKey[1];
            hdr[hdrLen++] = maskKey[2];
            hdr[hdrLen++] = maskKey[3];
        }

        client.write(hdr, hdrLen);
        if (payload && len > 0) {
            if (!maskFrame) {
                client.write(payload, len);
            } else {
                uint8_t chunk[64];
                for (size_t offset = 0; offset < len; offset += sizeof(chunk)) {
                    size_t n = (len - offset < sizeof(chunk)) ? (len - offset) : sizeof(chunk);
                    for (size_t j = 0; j < n; j++) {
                        chunk[j] = payload[offset + j] ^ maskKey[(offset + j) & 3];
                    }
                    client.write(chunk, n);
                }
            }
        }
        return true;
    }

    bool _readWsFrame(WiFiClient& client, WsMessageCallback cb, bool isClientSide) {
        if (client.available() < 2) return true;
        int b0 = client.read();
        int b1 = client.read();
        if (b0 < 0 || b1 < 0) return false;

        uint8_t opcode = (uint8_t)(b0 & 0x0F);
        bool masked = (b1 & 0x80) != 0;
        size_t payloadLen = (size_t)(b1 & 0x7F);

        if (payloadLen == 126) {
            unsigned long t = millis();
            while (client.available() < 2 && (millis() - t < 200)) delay(1);
            if (client.available() < 2) return false;
            payloadLen = ((size_t)client.read() << 8) | (size_t)client.read();
        } else if (payloadLen == 127) {
            return false; // Oversized frame not supported on MCU RAM
        }

        uint8_t maskKey[4] = {0, 0, 0, 0};
        if (masked) {
            unsigned long t = millis();
            while (client.available() < 4 && (millis() - t < 200)) delay(1);
            if (client.available() < 4) return false;
            for (int i = 0; i < 4; i++) maskKey[i] = (uint8_t)client.read();
        }

        if (opcode == 0x08) {
            return false; // Connection Close frame
        }

        if (payloadLen > 1024) payloadLen = 1024;
        char buf[1025];
        size_t readCount = 0;
        unsigned long start = millis();
        while (readCount < payloadLen && (millis() - start < 500)) {
            if (client.available()) {
                uint8_t byteVal = (uint8_t)client.read();
                if (masked) byteVal ^= maskKey[readCount & 3];
                buf[readCount++] = (char)byteVal;
            } else {
                delay(1);
            }
        }
        buf[readCount] = '\0';

        if (opcode == 0x09) {
            // Ping -> auto reply with Pong (0x0A)
            _sendWsFrame(client, 0x0A, (const uint8_t*)buf, readCount, isClientSide);
            return true;
        }

        if ((opcode == 0x01 || opcode == 0x02) && cb) {
            cb(buf);
        }
        return true;
    }
#endif // INTEGRALL_WEBSERVER_AVAILABLE
};

} // namespace Integrall

#endif // INTEGRALL_WEB_MODULE_H
