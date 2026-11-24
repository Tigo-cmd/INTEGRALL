import { useState } from "react";

/** Shows a device token once, with a ready-to-paste firmware snippet. */
export default function TokenReveal({ deviceId, token }: { deviceId: string; token: string }) {
  const [copied, setCopied] = useState(false);
  const broker = `mqtt://${location.hostname}:1883`;
  const snippet = `#define INTEGRALL_ENABLE_MQTT
#include <Integrall.h>
Integrall::System integrall;

void setup() {
  Integrall::DeviceConfig cfg;
  cfg.wifi_ssid     = "YourWiFi";
  cfg.wifi_password = "YourPassword";
  cfg.backend_url   = "${broker}";
  cfg.api_key       = "${token}";   // ${deviceId}
  integrall.begin(cfg);
}

void loop() { integrall.handle(); }`;
  const copy = async () => {
    await navigator.clipboard.writeText(snippet);
    setCopied(true);
    window.setTimeout(() => setCopied(false), 1500);
  };
  return (
    <div className="space-y-3">
      <p className="rounded-lg bg-amber-500/10 px-3 py-2 text-sm text-amber-200">
        Copy this token now — it is not stored here and can't be shown again (you can rotate it later).
      </p>
      <div>
        <div className="label mb-1">Device token</div>
        <code className="block break-all rounded-lg bg-ink-950 px-3 py-2 font-mono text-sm text-accent">{token}</code>
      </div>
      <div>
        <div className="mb-1 flex items-center justify-between">
          <span className="label">Firmware</span>
          <button className="btn-ghost py-1 text-xs" onClick={copy}>{copied ? "Copied" : "Copy sketch"}</button>
        </div>
        <pre className="max-h-64 overflow-auto rounded-lg bg-ink-950 p-3 font-mono text-xs text-slate-300">{snippet}</pre>
      </div>
    </div>
  );
}
