# OpenRemote as the Integrall UI

OpenRemote (open source, AGPL, github.com/openremote/core) is the dashboard. The Integrall
backend mirrors each device into it as an asset. Devices and firmware are unchanged.

```
ESP32/8266/Uno ──MQTT──▶ Mosquitto ──▶ Integrall backend ──REST + MQTT API──▶ OpenRemote (UI, rules, history)
   (frozen contract)        :1883        (device tokens, DB)   ◀── user writes ──
```

## 1. Start OpenRemote (its own stack)

Use OpenRemote's official compose file, pinned to the release this was written against (**1.34.1**):

```bash
mkdir -p ~/openremote && cd ~/openremote
curl -fsSLO https://raw.githubusercontent.com/openremote/core/1.34.1/docker-compose.yml
MANAGER_VERSION=1.34.1 OR_ADMIN_PASSWORD='change-me' docker compose -p openremote up -d
```

- UI: https://localhost (self-signed certificate). Log in as `admin` with the password you set (it defaults to `secret`).
- It uses ports **80, 443 and 8883**. It doesn't clash with Integrall (1883 / 8000).
- You need about 2–3 GB of free disk for the images.

## 2. Create a service user for the bridge

In the Manager UI, open **Users → Service users → Add**. Name it `integrall`.

- Give it at least **read:assets, write:assets, write:attributes** (or simply all the
  read/write roles while you're testing).
- Copy its **secret**.

## 3. Point the Integrall backend at it

Add these to `cloud/.env`, then run `docker compose up -d` in `cloud/`:

```
OR_URL=https://host.docker.internal      # backend in Docker, OpenRemote on the same host
OR_REALM=master
OR_CLIENT_ID=integrall
OR_CLIENT_SECRET=<secret from step 2>
OR_VERIFY_TLS=false                      # only for the quick-start self-signed cert
# OR_MQTT_HOST=host.docker.internal      # defaults to the host in OR_URL; port 8883 TLS
# OR_PARENT_ID=<asset id>                # optional: put all devices under one parent asset
```

To run the backend outside Docker, use the same values with the `INTEGRALL_` prefix
(for example `INTEGRALL_OR_URL=https://localhost`). To check that it connected, call
`GET http://localhost:8000/api/health`, which returns `"openremote": true`.

## 4. Add devices

```bash
python cloud/tools/integrall_devices.py --email you@example.com signup         # once
python cloud/tools/integrall_devices.py --email you@example.com add INT_A1B2C3D4E5F6 "Garage pump"
```

Put the printed token in the sketch (`DEVICE_TOKEN` in `examples/CloudMQTT`). An asset named
"Garage pump" appears in OpenRemote straight away, and fills in as the device reports.

## What each asset contains

| Attribute | Type | Source / effect |
|---|---|---|
| `online` | boolean (history) | status / LWT |
| `firmware`, `chip`, `board`, `ip`, `mac`, `modules` | text | meta |
| `health` (JSON), `partsMissing`, `rssi`, `heap`, `uptime` | – | health (`selfTest()`) |
| every telemetry key, e.g. `temperature` | number / boolean / text, with history | telemetry, created on first sight |
| `lastEvent` | text, with history | `sendEvent()` |
| `relay0`…`relay7` | boolean, **writable** | switch it in OpenRemote → `relay.set` on the device |
| `command` | JSON, **writable** | write `{"action":"buzzer.beep","args":{"ms":200}}` or just `"ping"` |
| `lastCommand` | JSON | `acked` / `failed` / `expired`, plus the device's message and result |

Notes:
- A relay switched in the UI snaps back if the device is offline or doesn't answer within 30 s.
- Telemetry keys that collide with the names above get a `t_` prefix (for example `t_relay0`).
- History, charts, rules, flows and maps all come from OpenRemote. Use **Insights** for
  dashboards and **Rules** for automations, such as "when `temperature` > 30, write `relay0` = true".
- Removing a device with `integrall_devices.py remove` also deletes its asset. If you delete
  an asset in OpenRemote, it's recreated the next time the backend connects.

## Verified against OpenRemote 1.34.1

`cloud/e2e/openremote_e2e.py` passes 20/20 checks against a live OpenRemote 1.34.1 stack.
It checks mirroring, live updates, relay switching, the `command` attribute (including
sending the same command twice), the offline snap-back, and asset deletion:

```bash
OR_CLIENT_SECRET=... python cloud/e2e/openremote_e2e.py --api http://localhost:8000 \
    --mqtt-host localhost --mqtt-port 1883 --or-url https://localhost --insecure
```

What the live run taught us (already handled in the code):
- Every `ThingAsset` must have `notes` (text) and `location` (GEO_JSONPoint). Set `location`
  in the UI to put the device on the map.
- `command` is of type `JSON`, not `JSONObject`. Otherwise OpenRemote rejects both a bare
  `"ping"` and the `null` used to clear the attribute.
- OpenRemote drops writes stamped older than the attribute's current timestamp. New attributes
  are therefore created already holding their first value.
- The service user has to be created in the UI. The `openremote` login client refuses
  password logins from scripts, and the manager doesn't accept Keycloak `admin-cli` tokens.
- Minimum roles for the service user: `read:assets`, `write:assets`, `write:attributes`.
