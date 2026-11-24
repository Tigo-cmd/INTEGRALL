/**
 * WebModule.h
 *
 * HTML dashboard renderer (HtmlBuilder), a local HTTP/REST server and RFC 6455
 * WebSockets (server + ws:// / wss:// client) for the Integrall Framework.
 *
 * Non-blocking by design (v1.2): every connection is a small state machine fed
 * from handle(); a slow or malicious client can never stall loop(). Route
 * handlers run in loop() context, so they may safely touch relays, LCD, I2C.
 *
 * Security defaults:
 *  - No CORS headers unless webEnableCORS() is called.
 *  - Cross-site request protection for handler routes: a request carrying an
 *    Origin/Referer from another site is rejected (403). Pages built with
 *    HtmlBuilder send "X-Integrall: 1", which other sites cannot forge.
 *    Scripts/apps (curl, Python) without Origin are allowed.
 *  - Optional HTTP Basic auth for everything: webSetAuth(user, pass).
 *  - HtmlBuilder escapes text arguments (labels, titles, values).
 *
 * Built on WiFiServer/WiFiClient: no extra libraries, and no clash with
 * esp_camera's esp_http_server (use different ports, e.g. camera on 80/81,
 * WebModule on 8080).
 */

#ifndef INTEGRALL_WEB_MODULE_H
#define INTEGRALL_WEB_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"

#if INTEGRALL_NETWORK_AVAILABLE
  #define INTEGRALL_WEBSERVER_AVAILABLE 1
  #if defined(ESP32)
    #include <WiFi.h>
  #elif defined(ESP8266)
    #include <ESP8266WiFi.h>
  #endif
  #include <WiFiClientSecure.h>
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

    /** Escape text for safe use inside HTML content or attribute values. */
    static String escape(const String& in) {
        String out;
        out.reserve(in.length() + 8);
        for (size_t i = 0; i < in.length(); i++) {
            const char c = in[i];
            switch (c) {
                case '&':  out += F("&amp;");  break;
                case '<':  out += F("&lt;");   break;
                case '>':  out += F("&gt;");   break;
                case '"':  out += F("&quot;"); break;
                case '\'': out += F("&#39;");  break;
                default:   out += c;
            }
        }
        return out;
    }

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
            inner += "<div class=\"ig-card-title\">" + escape(title) + "</div>";
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
        valSpan += ">" + escape(value) + "</span>";
        if (unit && strlen(unit) > 0) {
            valSpan += String("<span class=\"ig-metric-unit\">") + escape(unit) + "</span>";
        }
        String inner = "<div class=\"ig-metric-lbl\">" + escape(label) + "</div><div class=\"ig-metric-row\">" + valSpan + "</div>";
        return tag("div", inner, "class=\"ig-card ig-metric\"");
    }

    /**
     * Render a status pill badge ("ONLINE", "ALERT", etc.)
     * @param color "green", "red", "cyan", or "amber"
     */
    static String badge(const String& text, const char* color = "cyan") {
        String cls = String("class=\"ig-badge ig-badge-") + (color ? color : "cyan") + "\"";
        return tag("span", escape(text), cls.c_str());
    }

    /**
     * Render an interactive button that triggers an async fetch(endpoint) without page reload
     */
    static String button(const String& label, const char* endpoint, const char* color = "cyan") {
        String attr = String("class=\"ig-btn ig-btn-") + (color ? color : "cyan") + "\" onclick=\"igCmd('" + _jsAttr(endpoint ? endpoint : "/") + "')\"";
        return tag("button", escape(label), attr.c_str());
    }

    /**
     * Render an interactive toggle switch that calls endpoint?state=1 or endpoint?state=0
     */
    static String toggle(const String& label, const char* id, bool checked, const char* endpoint) {
        String html = "<div class=\"ig-ctrl-row\"><span>" + escape(label) + "</span>";
        html += "<input type=\"checkbox\" id=\"" + escape(id ? id : "sw") + "\"";
        if (checked) html += " checked";
        html += " onchange=\"igCmd('" + _jsAttr(endpoint ? endpoint : "/toggle") + "?state='+(this.checked?1:0))\"/>";
        html += "</div>";
        return html;
    }

    /**
     * Render an interactive range slider that calls endpoint?val=X
     */
    static String slider(const String& label, const char* id, int minVal, int maxVal, int curVal, const char* endpoint) {
        String cid = escape(id ? id : "rng");
        String html = "<div class=\"ig-ctrl-col\"><div class=\"ig-ctrl-row\"><span>" + escape(label) + "</span>";
        html += "<strong id=\"" + cid + "_v\">" + String(curVal) + "</strong></div>";
        html += "<input type=\"range\" class=\"ig-slider\" id=\"" + cid + "\" min=\"" + String(minVal) + "\" max=\"" + String(maxVal) + "\" value=\"" + String(curVal) + "\" ";
        html += "oninput=\"document.getElementById('" + cid + "_v').innerText=this.value\" ";
        html += "onchange=\"igCmd('" + _jsAttr(endpoint ? endpoint : "/set") + "?val='+this.value)\"/></div>";
        return html;
    }

    /**
     * Render a progress bar (0 - 100%)
     */
    static String progressBar(const String& label, int percent, const char* elementId = nullptr) {
        int pct = constrain(percent, 0, 100);
        String html = "<div class=\"ig-ctrl-col\"><div class=\"ig-ctrl-row\"><span>" + escape(label) + "</span><span>" + String(pct) + "%</span></div>";
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
        s += "ws.onmessage=function(e){try{var d=JSON.parse(e.data);for(var k in d){var el=document.getElementById(k);if(el)el.textContent=d[k];}}catch(_){";
        s += "var t=document.getElementById('" + _jsAttr(targetId ? targetId : "ws-log") + "');if(t)t.textContent=e.data;}};";
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
        doc += "<title>" + escape(title) + "</title><style>";
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
        // X-Integrall marks the request as coming from this device's own page (anti-CSRF)
        doc += "<script>function igCmd(u){fetch(u,{headers:{'X-Integrall':'1'}}).catch(function(e){console.error(e);});}</script></body></html>";
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

    /** Make a string safe inside a JS single-quoted literal inside an HTML attribute. */
    static String _jsAttr(const char* in) {
        String out;
        for (const char* p = in; p && *p; p++) {
            if (*p == '\'' || *p == '\\') { out += '\\'; out += *p; }
            else if (*p == '"') out += F("&quot;");
            else if (*p == '<') out += F("\\x3c");
            else out += *p;
        }
        return out;
    }
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
// 3. LOCAL HTTP/REST SERVER + RFC 6455 WEBSOCKETS (non-blocking engine)
// ============================================================================
typedef void (*WebRouteCallback)();
typedef void (*WsMessageCallback)(const char* message);

#ifndef INTEGRALL_WEB_MAX_ROUTES
  #define INTEGRALL_WEB_MAX_ROUTES 16
#endif
#ifndef INTEGRALL_WEB_MAX_CONN
  #define INTEGRALL_WEB_MAX_CONN 2          // simultaneous HTTP requests being received
#endif
#ifndef INTEGRALL_WEB_MAX_BODY
  #define INTEGRALL_WEB_MAX_BODY 2048       // larger bodies get 413
#endif
#ifndef INTEGRALL_WS_MAX_CLIENTS
  #if defined(ESP8266)
    #define INTEGRALL_WS_MAX_CLIENTS 2
  #else
    #define INTEGRALL_WS_MAX_CLIENTS 4
  #endif
#endif
#ifndef INTEGRALL_WS_MAX_MSG
  #define INTEGRALL_WS_MAX_MSG 1024         // larger messages close the socket with 1009
#endif

#if INTEGRALL_WEBSERVER_AVAILABLE
namespace Detail {
inline uint32_t random32() {
    #if defined(ESP32)
    return esp_random();
    #else
    return ESP.random();
    #endif
}

/** Case-insensitive "Name:" header match; returns the trimmed value or "". */
inline String headerValue(const String& headers, const char* name) {
    const size_t n = strlen(name);
    int from = 0;
    while (from < (int)headers.length()) {
        int eol = headers.indexOf('\n', from);
        if (eol < 0) eol = headers.length();
        if ((int)(eol - from) > (int)n && headers[from + n] == ':' &&
            strncasecmp(headers.c_str() + from, name, n) == 0) {
            String v = headers.substring(from + n + 1, eol);
            v.trim();
            return v;
        }
        from = eol + 1;
    }
    return String();
}

/** "host:port" -> "host". */
inline String hostnameOnly(const String& hp) {
    int c = hp.lastIndexOf(':');
    return (c > 0 && hp.indexOf(']') < c) ? hp.substring(0, c) : hp;
}

/** Host part of "http://host:port/..." (keeps :port). */
inline String hostOfUrl(const String& url) {
    int s = url.indexOf("://");
    if (s < 0) return String();
    s += 3;
    int e = url.indexOf('/', s);
    return url.substring(s, e < 0 ? url.length() : e);
}

inline String urlDecode(const String& in) {
    String out;
    out.reserve(in.length());
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        if (c == '+') { out += ' '; continue; }
        if (c == '%' && i + 2 < in.length()) {
            char h[3] = {in[i + 1], in[i + 2], 0};
            out += (char)strtol(h, nullptr, 16);
            i += 2;
            continue;
        }
        out += c;
    }
    return out;
}
} // namespace Detail
#endif

class WebModule {
public:
    WebModule() {}
    ~WebModule() {
        #if INTEGRALL_WEBSERVER_AVAILABLE
        if (_http_server) { _http_server->stop(); delete _http_server; }
        if (_ws_server) { _ws_server->stop(); delete _ws_server; }
        for (uint8_t i = 0; i < INTEGRALL_WS_MAX_CLIENTS; i++) delete _wsc[i];
        delete _wsc_peer;
        #if defined(ESP8266)
        delete _wsc_x509;
        #endif
        #endif
    }

#if INTEGRALL_WEBSERVER_AVAILABLE
    // ======================================================================
    // HTTP server
    // ======================================================================
    /** Start the HTTP server (default port 80). Idempotent. */
    bool beginServer(uint16_t port = 80) {
        if (_http_server) return true;
        _http_server = new WiFiServer(port);
        _http_server->begin();
        #if defined(ESP8266)
        _http_server->setNoDelay(true);
        #endif
        INTEGRALL_LOG_INFO_VAL("Web: HTTP server on port ", port);
        return true;
    }

    void onGet(const char* uri, WebRouteCallback h)    { _addRoute(M_GET, uri, h, String(), nullptr); }
    void onPost(const char* uri, WebRouteCallback h)   { _addRoute(M_POST, uri, h, String(), nullptr); }
    void onPut(const char* uri, WebRouteCallback h)    { _addRoute(M_PUT, uri, h, String(), nullptr); }
    void onPatch(const char* uri, WebRouteCallback h)  { _addRoute(M_PATCH, uri, h, String(), nullptr); }
    void onDelete(const char* uri, WebRouteCallback h) { _addRoute(M_DELETE, uri, h, String(), nullptr); }
    /** Any method. */
    void on(const char* uri, WebRouteCallback h)       { _addRoute(M_ANY, uri, h, String(), nullptr); }

    /** Serve a fixed HTML page (read-only, always allowed). */
    void servePage(const char* uri, const String& html) { _addRoute(M_GET, uri, nullptr, html, "text/html; charset=utf-8"); }
    /** Serve a fixed JSON document (read-only, always allowed). */
    void serveJson(const char* uri, const String& json) { _addRoute(M_GET, uri, nullptr, json, "application/json"); }

    /** Require HTTP Basic auth for every request (nullptr to disable). */
    void setAuth(const char* user, const char* pass) {
        if (!user || !pass) { _auth_b64 = String(); return; }
        String up = String(user) + ":" + pass;
        _auth_b64 = Detail::base64Encode((const uint8_t*)up.c_str(), up.length());
    }

    /**
     * Allow cross-origin browser access (adds CORS headers and disables the
     * cross-site check). Pass the exact origin (e.g. "https://my.app") or "*".
     */
    void enableCORS(const char* origin = "*") { _cors_origin = origin ? origin : ""; }

    /** Disable the cross-site request check (not recommended). */
    void allowCrossSite(bool allow = true) { _allow_cross_site = allow; }

    // ---- inside a route handler ---------------------------------------------
    String arg(const char* name) const {
        if (!name) return String();
        for (uint8_t i = 0; i < _arg_count; i++) if (_arg_keys[i].equalsIgnoreCase(name)) return _arg_vals[i];
        return String();
    }
    bool hasArg(const char* name) const {
        for (uint8_t i = 0; i < _arg_count; i++) if (_arg_keys[i].equalsIgnoreCase(name)) return true;
        return false;
    }
    const String& body() const   { return _req_body; }
    const String& method() const { return _req_method; }
    const String& uri() const    { return _req_path; }
    /** Request header (case-insensitive), "" if absent. */
    String header(const char* name) const { return Detail::headerValue(_req_headers, name); }

    void sendHtml(const String& html, int code = 200) { _respond(code, "text/html; charset=utf-8", html); }
    void sendJson(const String& json, int code = 200) { _respond(code, "application/json", json); }
    void sendText(const String& text, int code = 200) { _respond(code, "text/plain; charset=utf-8", text); }
    void sendRedirect(const char* location, int code = 302) {
        if (!_cur || !location) return;
        _extra_header = String("Location: ") + location + "\r\n";
        _respond(code, "text/plain", String());
    }

    // ======================================================================
    // WebSocket server
    // ======================================================================
    bool beginWsServer(uint16_t port = 81) {
        if (_ws_server) return true;
        for (uint8_t i = 0; i < INTEGRALL_WS_MAX_CLIENTS; i++) if (!_wsc[i]) _wsc[i] = new WsPeer();
        _ws_server = new WiFiServer(port);
        _ws_server->begin();
        INTEGRALL_LOG_INFO_VAL("Web: WebSocket server on port ", port);
        return true;
    }
    void onWsMessage(WsMessageCallback cb) { _ws_msg_cb = cb; }

    /** Send a text frame to every connected client. @return clients reached. */
    uint8_t wsBroadcast(const char* message) {
        if (!message) return 0;
        uint8_t sent = 0;
        const size_t len = strlen(message);
        for (uint8_t i = 0; i < INTEGRALL_WS_MAX_CLIENTS; i++) {
            if (_wsc[i] && _wsc[i]->open && _wsc[i]->client.connected() &&
                _sendFrame(_wsc[i]->client, 0x1, (const uint8_t*)message, len, false)) sent++;
        }
        return sent;
    }

    uint8_t wsClientCount() const {
        uint8_t n = 0;
        for (uint8_t i = 0; i < INTEGRALL_WS_MAX_CLIENTS; i++) if (_wsc[i] && _wsc[i]->open) n++;
        return n;
    }

    // ======================================================================
    // WebSocket client (ws:// and wss://), auto-reconnect with back-off
    // ======================================================================
    /**
     * Connect to ws://host[:port]/path or wss://host[:port]/path. The TCP/TLS
     * connect itself is bounded (3 s); the upgrade handshake then completes
     * in the background. Stays connected (auto-reconnects) until wsDisconnect().
     */
    bool wsConnect(const char* url) {
        if (!url) return false;
        bool secure = false;
        const char* p = url;
        if (strncmp(p, "wss://", 6) == 0) { secure = true; p += 6; }
        else if (strncmp(p, "ws://", 5) == 0) p += 5;
        String rest(p);
        int slash = rest.indexOf('/');
        String hostPort = slash >= 0 ? rest.substring(0, slash) : rest;
        String path = slash >= 0 ? rest.substring(slash) : String("/");
        uint16_t port = secure ? 443 : 80;
        int colon = hostPort.indexOf(':');
        String host = hostPort;
        if (colon >= 0) { host = hostPort.substring(0, colon); port = (uint16_t)hostPort.substring(colon + 1).toInt(); }
        return _wsClientStart(host, port, path, secure);
    }

    bool wsConnect(const char* host, uint16_t port, const char* path = "/") {
        if (!host) return false;
        return _wsClientStart(String(host), port, String(path ? path : "/"), false);
    }

    /** Pin a root CA for wss:// (default: encrypted but not verified). */
    void wsSetCACert(const char* rootCA) { _wsc_ca = rootCA; }

    bool wsSend(const char* message) {
        if (!message || _wsc_state != WSC_OPEN) return false;
        return _sendFrame(*_wsc_sock, 0x1, (const uint8_t*)message, strlen(message), true);
    }
    void onWsClientMessage(WsMessageCallback cb) { _wsc_msg_cb = cb; }
    bool wsConnected() const { return _wsc_state == WSC_OPEN; }
    void wsDisconnect() {
        if (_wsc_state == WSC_OPEN) _sendFrame(*_wsc_sock, 0x8, nullptr, 0, true);
        if (_wsc_sock) _wsc_sock->stop();
        _wsc_state = WSC_IDLE;
        _wsc_auto = false;
    }
#endif // INTEGRALL_WEBSERVER_AVAILABLE

    /** Called by System::handle(). Never blocks. */
    void handle() {
        #if INTEGRALL_WEBSERVER_AVAILABLE
        if (_http_server) _handleHttp();
        if (_ws_server) _handleWsServer();
        _handleWsClient();
        #endif
    }

private:
#if INTEGRALL_WEBSERVER_AVAILABLE
    enum Method : uint8_t { M_GET, M_POST, M_PUT, M_PATCH, M_DELETE, M_ANY, M_OTHER };

    struct Route {
        Method           method = M_GET;
        String           uri;
        WebRouteCallback handler = nullptr;
        String           payload;
        const char*      mime = nullptr;
    };

    enum ConnStage : uint8_t { C_FREE, C_HEAD, C_BODY };
    struct Conn {
        WiFiClient    client;
        ConnStage     stage = C_FREE;
        String        buf;
        size_t        need = 0;          // body bytes still expected
        unsigned long since = 0;
    };

    struct WsPeer {
        WiFiClient    client;
        bool          used = false;
        bool          open = false;      // handshake complete
        String        head;              // handshake accumulation
        uint8_t       rx[INTEGRALL_WS_MAX_MSG + 14];
        size_t        rx_len = 0;
        char          msg[INTEGRALL_WS_MAX_MSG + 1];
        size_t        msg_len = 0;       // fragmented message assembly
        unsigned long since = 0;
    };

    // HTTP state
    WiFiServer* _http_server = nullptr;
    Route       _routes[INTEGRALL_WEB_MAX_ROUTES];
    uint8_t     _route_count = 0;
    Conn        _conn[INTEGRALL_WEB_MAX_CONN];
    WiFiClient* _cur = nullptr;          // client of the request being handled
    bool        _responded = false;
    String      _req_method, _req_path, _req_headers, _req_body, _extra_header;
    static const uint8_t MAX_ARGS = 8;
    String      _arg_keys[MAX_ARGS], _arg_vals[MAX_ARGS];
    uint8_t     _arg_count = 0;
    String      _auth_b64;
    String      _cors_origin;
    bool        _allow_cross_site = false;

    // WebSocket server state
    WiFiServer*       _ws_server = nullptr;
    WsPeer*           _wsc[INTEGRALL_WS_MAX_CLIENTS] = {nullptr};   // allocated by beginWsServer()
    WsMessageCallback _ws_msg_cb = nullptr;

    // WebSocket client state
    enum WscState : uint8_t { WSC_IDLE, WSC_HANDSHAKE, WSC_OPEN, WSC_WAIT_RETRY };
    WscState          _wsc_state = WSC_IDLE;
    bool              _wsc_auto = false;
    bool              _wsc_secure = false;
    String            _wsc_host, _wsc_path, _wsc_key;
    uint16_t          _wsc_port = 80;
    WiFiClient        _wsc_plain;
    WiFiClientSecure  _wsc_tls;
    Client*           _wsc_sock = nullptr;
    const char*       _wsc_ca = nullptr;
    #if defined(ESP8266)
    BearSSL::X509List* _wsc_x509 = nullptr;
    #endif
    WsPeer*           _wsc_peer = nullptr;      // reuse the frame parser buffers
    unsigned long     _wsc_since = 0;
    uint32_t          _wsc_backoff = 2000;
    WsMessageCallback _wsc_msg_cb = nullptr;

    // ---------------------------------------------------------------- HTTP --
    static Method _parseMethod(const String& m) {
        if (m == "GET") return M_GET;
        if (m == "POST") return M_POST;
        if (m == "PUT") return M_PUT;
        if (m == "PATCH") return M_PATCH;
        if (m == "DELETE") return M_DELETE;
        return M_OTHER;
    }

    void _addRoute(Method m, const char* uri, WebRouteCallback h, const String& payload, const char* mime) {
        if (!uri) return;
        if (!_http_server) beginServer(80);
        for (uint8_t i = 0; i < _route_count; i++) {
            if (_routes[i].method == m && _routes[i].uri == uri) {
                _routes[i].handler = h; _routes[i].payload = payload; _routes[i].mime = mime;
                return;
            }
        }
        if (_route_count >= INTEGRALL_WEB_MAX_ROUTES) {
            INTEGRALL_LOG_ERROR("Web: route table full (raise INTEGRALL_WEB_MAX_ROUTES)");
            return;
        }
        Route& r = _routes[_route_count++];
        r.method = m; r.uri = uri; r.handler = h; r.payload = payload; r.mime = mime;
    }

    static WiFiClient _accept(WiFiServer* srv) {
        #if defined(ESP32) && defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        return srv->accept();
        #elif defined(ESP8266)
        return srv->accept();
        #else
        return srv->available();
        #endif
    }

    void _handleHttp() {
        // Accept new connections into free slots
        WiFiClient incoming = _accept(_http_server);
        if (incoming) {
            bool placed = false;
            for (uint8_t i = 0; i < INTEGRALL_WEB_MAX_CONN && !placed; i++) {
                if (_conn[i].stage == C_FREE) {
                    _conn[i].client = incoming;
                    _conn[i].stage = C_HEAD;
                    _conn[i].buf = String();
                    _conn[i].buf.reserve(256);
                    _conn[i].need = 0;
                    _conn[i].since = millis();
                    placed = true;
                }
            }
            if (!placed) {
                incoming.print(F("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"));
                incoming.stop();
            }
        }

        const unsigned long now = millis();
        for (uint8_t i = 0; i < INTEGRALL_WEB_MAX_CONN; i++) {
            Conn& c = _conn[i];
            if (c.stage == C_FREE) continue;
            if (!c.client.connected() && !c.client.available()) { _close(c); continue; }
            if (now - c.since > 3000) { _quick(c, 408, "Request Timeout"); continue; }

            int budget = 512;                               // bounded work per loop
            while (budget-- > 0 && c.client.available() > 0) {
                char ch = (char)c.client.read();
                if (c.stage == C_HEAD) {
                    c.buf += ch;
                    if (c.buf.length() > 2048) { _quick(c, 431, "Request Header Fields Too Large"); break; }
                    if (c.buf.endsWith("\r\n\r\n")) {
                        String cl = Detail::headerValue(c.buf, "Content-Length");
                        long n = cl.length() ? cl.toInt() : 0;
                        if (n > INTEGRALL_WEB_MAX_BODY) { _quick(c, 413, "Payload Too Large"); break; }
                        c.need = (size_t)(n > 0 ? n : 0);
                        c.stage = C_BODY;
                        _req_body = String();
                        _req_body.reserve(c.need);
                        if (c.need == 0) break;
                    }
                } else {
                    _req_body += ch;
                    if (--c.need == 0) break;
                }
            }
            if (c.stage == C_BODY && c.need == 0) _dispatch(c);
        }
    }

    void _dispatch(Conn& c) {
        // Request line + headers live in c.buf
        int eol = c.buf.indexOf("\r\n");
        String line = c.buf.substring(0, eol);
        _req_headers = c.buf.substring(eol + 2);
        int sp1 = line.indexOf(' '), sp2 = line.indexOf(' ', sp1 + 1);
        if (sp1 < 0 || sp2 < 0) { _quick(c, 400, "Bad Request"); return; }
        _req_method = line.substring(0, sp1);
        String full = line.substring(sp1 + 1, sp2);
        int q = full.indexOf('?');
        _req_path = Detail::urlDecode(q >= 0 ? full.substring(0, q) : full);
        _arg_count = 0;
        if (q >= 0) _parseArgs(full.substring(q + 1));
        if (header("Content-Type").startsWith("application/x-www-form-urlencoded")) _parseArgs(_req_body);

        _cur = &c.client;
        _responded = false;
        _extra_header = String();

        if (_auth_b64.length() && header("Authorization") != String("Basic ") + _auth_b64) {
            _extra_header = F("WWW-Authenticate: Basic realm=\"Integrall\"\r\n");
            _respond(401, "text/plain", F("Unauthorized"));
        } else if (_req_method == "OPTIONS") {
            _respond(_cors_origin.length() ? 204 : 405, "text/plain", String());
        } else {
            const Method m = _parseMethod(_req_method);
            Route* match = nullptr;
            bool pathExists = false;
            for (uint8_t i = 0; i < _route_count; i++) {
                if (_routes[i].uri != _req_path) continue;
                pathExists = true;
                if (_routes[i].method == m || _routes[i].method == M_ANY) { match = &_routes[i]; break; }
            }
            if (!match) {
                _respond(pathExists ? 405 : 404, "application/json",
                         pathExists ? F("{\"error\":\"method not allowed\"}") : F("{\"error\":\"not found\"}"));
            } else if (match->handler) {
                if (_isCrossSite()) {
                    INTEGRALL_LOG_WARN("Web: blocked cross-site request (send header X-Integrall: 1, or webAllowCrossSite)");
                    _respond(403, "application/json", F("{\"error\":\"cross-site request blocked\"}"));
                } else {
                    match->handler();
                    if (!_responded) _respond(200, "text/plain", F("OK"));
                }
            } else {
                _respond(200, match->mime ? match->mime : "text/plain", match->payload);
            }
        }
        _cur = nullptr;
        _close(c);
    }

    /** true if a browser on another site sent this (Origin/Referer host != Host). */
    bool _isCrossSite() const {
        if (_allow_cross_site || _cors_origin.length()) return false;
        if (header("X-Integrall").length()) return false;   // custom header: only same-origin JS can set it
        const String host = header("Host");
        const String origin = header("Origin");
        if (origin.length()) return Detail::hostOfUrl(origin) != host;
        const String ref = header("Referer");
        if (ref.length()) return Detail::hostOfUrl(ref) != host;
        return false;                                        // non-browser client (curl, app)
    }

    void _parseArgs(const String& qs) {
        int start = 0;
        while (start < (int)qs.length() && _arg_count < MAX_ARGS) {
            int amp = qs.indexOf('&', start);
            String pair = (amp >= 0) ? qs.substring(start, amp) : qs.substring(start);
            int eq = pair.indexOf('=');
            if (eq > 0) {
                _arg_keys[_arg_count] = Detail::urlDecode(pair.substring(0, eq));
                _arg_vals[_arg_count] = Detail::urlDecode(pair.substring(eq + 1));
                _arg_count++;
            } else if (pair.length()) {
                _arg_keys[_arg_count] = Detail::urlDecode(pair);
                _arg_vals[_arg_count] = String();
                _arg_count++;
            }
            if (amp < 0) break;
            start = amp + 1;
        }
    }

    static const char* _reason(int code) {
        switch (code) {
            case 200: return "OK";             case 201: return "Created";
            case 204: return "No Content";     case 301: return "Moved Permanently";
            case 302: return "Found";          case 400: return "Bad Request";
            case 401: return "Unauthorized";   case 403: return "Forbidden";
            case 404: return "Not Found";      case 405: return "Method Not Allowed";
            case 408: return "Request Timeout";case 413: return "Payload Too Large";
            case 431: return "Request Header Fields Too Large";
            case 500: return "Internal Server Error"; case 503: return "Service Unavailable";
            default:  return "OK";
        }
    }

    void _respond(int code, const char* type, const String& content) {
        if (!_cur || _responded) return;
        String h;
        h.reserve(160);
        h += "HTTP/1.1 "; h += code; h += ' '; h += _reason(code); h += "\r\n";
        h += "Content-Type: "; h += type ? type : "text/plain"; h += "\r\n";
        h += "Content-Length: "; h += content.length(); h += "\r\n";
        h += "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n";
        if (_cors_origin.length()) {
            h += "Access-Control-Allow-Origin: "; h += _cors_origin; h += "\r\n";
            h += "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n";
            h += "Access-Control-Allow-Headers: Content-Type, Authorization, X-Integrall\r\n";
        }
        h += _extra_header;
        h += "Connection: close\r\n\r\n";
        _cur->print(h);
        if (content.length()) _cur->print(content);
        _responded = true;
    }

    void _quick(Conn& c, int code, const char* text) {
        c.client.print(String("HTTP/1.1 ") + code + " " + text + "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        _close(c);
    }

    void _close(Conn& c) {
        c.client.stop();
        c.stage = C_FREE;
        c.buf = String();
    }

    // ----------------------------------------------------------- WS server --
    void _handleWsServer() {
        WiFiClient incoming = _accept(_ws_server);
        if (incoming) {
            WsPeer* slot = nullptr;
            for (uint8_t i = 0; i < INTEGRALL_WS_MAX_CLIENTS && !slot; i++) if (_wsc[i] && !_wsc[i]->used) slot = _wsc[i];
            if (!slot) {
                incoming.print(F("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\n\r\n"));
                incoming.stop();
            } else {
                slot->client = incoming; slot->used = true; slot->open = false;
                slot->head = String(); slot->rx_len = 0; slot->msg_len = 0; slot->since = millis();
            }
        }
        for (uint8_t i = 0; i < INTEGRALL_WS_MAX_CLIENTS; i++) {
            if (!_wsc[i] || !_wsc[i]->used) continue;
            WsPeer& p = *_wsc[i];
            if (!p.client.connected() && !p.client.available()) { _wsDrop(p); continue; }
            if (!p.open) {
                if (millis() - p.since > 3000) { _wsDrop(p); continue; }
                int budget = 512;
                while (budget-- > 0 && p.client.available() > 0) {
                    p.head += (char)p.client.read();
                    if (p.head.length() > 2048) { _wsDrop(p); break; }
                    if (p.head.endsWith("\r\n\r\n")) { _wsServerHandshake(p); break; }
                }
            } else if (!_pumpFrames(p, p.client, false, _ws_msg_cb)) {
                _wsDrop(p);
            }
        }
    }

    void _wsServerHandshake(WsPeer& p) {
        String key = Detail::headerValue(p.head, "Sec-WebSocket-Key");
        String upgrade = Detail::headerValue(p.head, "Upgrade");
        String origin = Detail::headerValue(p.head, "Origin");
        String host = Detail::headerValue(p.head, "Host");
        // Page is usually on :80 and the socket on :81, so compare hostnames only.
        const bool crossSite = origin.length() && !_allow_cross_site && !_cors_origin.length() &&
                               Detail::hostnameOnly(Detail::hostOfUrl(origin)) != Detail::hostnameOnly(host);
        if (!key.length() || !upgrade.equalsIgnoreCase("websocket") || crossSite) {
            p.client.print(crossSite ? F("HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n")
                                     : F("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"));
            _wsDrop(p);
            return;
        }
        String resp = F("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ");
        resp += Detail::computeWsAcceptKey(key);
        resp += "\r\n\r\n";
        p.client.print(resp);
        p.open = true;
        p.head = String();
    }

    void _wsDrop(WsPeer& p) {
        p.client.stop();
        p.used = false; p.open = false;
        p.head = String(); p.rx_len = 0; p.msg_len = 0;
    }

    /**
     * Read available bytes into the peer buffer and process every complete
     * frame. Returns false if the connection must be closed.
     * isClient: we are the client side (our frames masked, theirs not).
     */
    bool _pumpFrames(WsPeer& p, Client& sock, bool isClient, WsMessageCallback cb) {
        int budget = 512;
        while (budget-- > 0 && sock.available() > 0 && p.rx_len < sizeof(p.rx)) {
            p.rx[p.rx_len++] = (uint8_t)sock.read();
        }
        for (;;) {
            if (p.rx_len < 2) return true;
            const uint8_t b0 = p.rx[0], b1 = p.rx[1];
            const bool fin = b0 & 0x80;
            const uint8_t op = b0 & 0x0F;
            const bool masked = b1 & 0x80;
            if (!isClient && !masked) { _wsClose(sock, 1002, isClient); return false; }   // RFC: client frames must be masked
            size_t len = b1 & 0x7F, hdr = 2;
            if (len == 126) {
                if (p.rx_len < 4) return true;
                len = ((size_t)p.rx[2] << 8) | p.rx[3]; hdr = 4;
            } else if (len == 127) {
                _wsClose(sock, 1009, isClient); return false;                           // > 64 KB: never on an MCU
            }
            if (masked) hdr += 4;
            if (len > INTEGRALL_WS_MAX_MSG) { _wsClose(sock, 1009, isClient); return false; }
            if (p.rx_len < hdr + len) return true;                                      // wait for the rest
            uint8_t* payload = p.rx + hdr;
            if (masked) { const uint8_t* mk = p.rx + hdr - 4; for (size_t i = 0; i < len; i++) payload[i] ^= mk[i & 3]; }

            if (op == 0x8) { _wsClose(sock, 1000, isClient); return false; }
            if (op == 0x9) { _sendFrame(sock, 0xA, payload, len, isClient); }
            else if (op == 0x1 || op == 0x2 || op == 0x0) {
                if (op != 0x0) p.msg_len = 0;
                if (p.msg_len + len > INTEGRALL_WS_MAX_MSG) { _wsClose(sock, 1009, isClient); return false; }
                memcpy(p.msg + p.msg_len, payload, len);
                p.msg_len += len;
                if (fin) {
                    p.msg[p.msg_len] = '\0';
                    if (cb) cb(p.msg);
                    p.msg_len = 0;
                }
            }
            // consume this frame
            const size_t used = hdr + len;
            memmove(p.rx, p.rx + used, p.rx_len - used);
            p.rx_len -= used;
        }
    }

    bool _sendFrame(Client& sock, uint8_t op, const uint8_t* data, size_t len, bool mask) {
        if (!sock.connected() || len > 65535) return false;
        uint8_t h[14]; size_t n = 0;
        h[n++] = 0x80 | (op & 0x0F);
        const uint8_t mbit = mask ? 0x80 : 0x00;
        if (len <= 125) h[n++] = mbit | (uint8_t)len;
        else { h[n++] = mbit | 126; h[n++] = (uint8_t)(len >> 8); h[n++] = (uint8_t)len; }
        uint8_t mk[4] = {0, 0, 0, 0};
        if (mask) {
            uint32_t r = Detail::random32();                 // RFC 6455: fresh random key per frame
            memcpy(mk, &r, 4);
            memcpy(h + n, mk, 4); n += 4;
        }
        sock.write(h, n);
        if (!len) return true;
        if (!mask) { sock.write(data, len); return true; }
        uint8_t chunk[64];
        for (size_t off = 0; off < len; off += sizeof(chunk)) {
            size_t k = (len - off < sizeof(chunk)) ? len - off : sizeof(chunk);
            for (size_t j = 0; j < k; j++) chunk[j] = data[off + j] ^ mk[(off + j) & 3];
            sock.write(chunk, k);
        }
        return true;
    }

    void _wsClose(Client& sock, uint16_t code, bool mask) {
        uint8_t c[2] = {(uint8_t)(code >> 8), (uint8_t)code};
        _sendFrame(sock, 0x8, c, 2, mask);
    }

    // ----------------------------------------------------------- WS client --
    bool _wsClientStart(const String& host, uint16_t port, const String& path, bool secure) {
        _wsc_host = host; _wsc_port = port; _wsc_path = path; _wsc_secure = secure;
        _wsc_auto = true;
        _wsc_backoff = 2000;
        return _wsClientOpen();
    }

    bool _wsClientOpen() {
        if (!_wsc_peer) { _wsc_peer = new WsPeer(); }
        _wsc_peer->rx_len = 0; _wsc_peer->msg_len = 0; _wsc_peer->head = String();
        if (_wsc_secure) {
            #if defined(ESP8266)
            if (_wsc_ca) {
                delete _wsc_x509;
                _wsc_x509 = new BearSSL::X509List(_wsc_ca);
                _wsc_tls.setTrustAnchors(_wsc_x509);
            } else {
                _wsc_tls.setInsecure();
            }
            #else
            if (_wsc_ca) _wsc_tls.setCACert(_wsc_ca); else _wsc_tls.setInsecure();
            #endif
            _wsc_sock = &_wsc_tls;
        } else {
            _wsc_sock = &_wsc_plain;
        }
        _wsc_sock->stop();
        #if defined(ESP32)
        bool ok = (_wsc_secure ? _wsc_tls.connect(_wsc_host.c_str(), _wsc_port, 3000)
                               : _wsc_plain.connect(_wsc_host.c_str(), _wsc_port, 3000));
        #else
        _wsc_plain.setTimeout(3000); _wsc_tls.setTimeout(3000);
        bool ok = _wsc_sock->connect(_wsc_host.c_str(), _wsc_port);
        #endif
        if (!ok) {
            INTEGRALL_LOG_WARN_VAL("WS client: connect failed to ", _wsc_host);
            _wsc_state = WSC_WAIT_RETRY; _wsc_since = millis();
            return false;
        }
        uint8_t nonce[16];
        for (uint8_t i = 0; i < 16; i += 4) { uint32_t r = Detail::random32(); memcpy(nonce + i, &r, 4); }
        _wsc_key = Detail::base64Encode(nonce, 16);
        String req = String("GET ") + _wsc_path + " HTTP/1.1\r\nHost: " + _wsc_host + ":" + _wsc_port +
                     "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + _wsc_key +
                     "\r\nSec-WebSocket-Version: 13\r\n\r\n";
        _wsc_sock->print(req);
        _wsc_state = WSC_HANDSHAKE;
        _wsc_since = millis();
        return true;
    }

    void _handleWsClient() {
        if (_wsc_state == WSC_IDLE || !_wsc_sock) return;
        const unsigned long now = millis();
        if (_wsc_state == WSC_WAIT_RETRY) {
            if (_wsc_auto && now - _wsc_since >= _wsc_backoff) {
                _wsc_backoff = _wsc_backoff >= 30000 ? 60000 : _wsc_backoff * 2;
                _wsClientOpen();
            }
            return;
        }
        if (!_wsc_sock->connected() && !_wsc_sock->available()) {
            INTEGRALL_LOG_WARN("WS client: connection lost");
            _wsc_state = WSC_WAIT_RETRY; _wsc_since = now;
            return;
        }
        if (_wsc_state == WSC_HANDSHAKE) {
            if (now - _wsc_since > 5000) { _wsc_sock->stop(); _wsc_state = WSC_WAIT_RETRY; _wsc_since = now; return; }
            int budget = 512;
            while (budget-- > 0 && _wsc_sock->available() > 0) {
                _wsc_peer->head += (char)_wsc_sock->read();
                if (!_wsc_peer->head.endsWith("\r\n\r\n")) continue;
                const String& h = _wsc_peer->head;
                String accept = Detail::headerValue(h.substring(h.indexOf("\r\n") + 2), "Sec-WebSocket-Accept");
                if (h.startsWith("HTTP/1.1 101") && accept == Detail::computeWsAcceptKey(_wsc_key)) {
                    _wsc_state = WSC_OPEN;
                    _wsc_backoff = 2000;
                    INTEGRALL_LOG_INFO_VAL("WS client: connected to ", _wsc_host);
                } else {
                    INTEGRALL_LOG_ERROR("WS client: handshake rejected");
                    _wsc_sock->stop(); _wsc_state = WSC_WAIT_RETRY; _wsc_since = now;
                }
                _wsc_peer->head = String();
                break;
            }
            return;
        }
        if (!_pumpFrames(*_wsc_peer, *_wsc_sock, true, _wsc_msg_cb)) {
            _wsc_sock->stop();
            _wsc_state = WSC_WAIT_RETRY; _wsc_since = now;
        }
    }
#endif // INTEGRALL_WEBSERVER_AVAILABLE
};

} // namespace Integrall

#endif // INTEGRALL_WEB_MODULE_H
