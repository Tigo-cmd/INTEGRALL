# INTEGRALL — Engineering Handoff (living document)

> **Read this first** if you are an agent/developer picking up this work.
> Single source of truth for the v1.2 upgrade: decisions, what was found,
> what is done, how to verify, and what is left. Update §7 (status) and
> §9 (log) after every meaningful step. See also CHANGELOG.md.

Last updated: 2026-11-23 (session 1, end)

---

## 1. Goal (v1.1.0 → v1.2.0)

1. Firmware fully implemented: **no blocking waits, no polling**, no silent
   stubs, builds clean on ESP32 (core 2 + core 3), ESP8266, Uno/Nano.
2. Verifiable **without the parts**: compile matrix, host unit tests, Wokwi
   simulation, on-device `selfTest()`, a simulated cloud device.
3. IoT cloud: **MQTT** transport, **FastAPI + Postgres** backend, **React dashboard**.
4. Guided tests for the owner's real hardware.

## 2. Locked decisions (don't re-litigate)

| Topic | Decision |
|---|---|
| Build tool | PlatformIO (Arduino IDE must keep working: library.properties kept) |
| Device ↔ cloud | MQTT (Mosquitto 2 + dynamic-security). Commands pushed, never polled. Clean session: commands never replayed after an outage. |
| Backend | FastAPI + async SQLAlchemy + Postgres, Alembic migrations |
| UI | **OpenRemote** (open source, github.com/openremote/core, pinned 1.34.1), run as its own stack. The backend mirrors devices into it (`cloud/backend/app/openremote.py`); firmware unchanged. The React `cloud/dashboard` is **retired** (compose profile `legacy-dashboard`, kept for reference). Devices are registered with `cloud/tools/integrall_devices.py`. |
| Hardware on hand | ESP32 DevKit, ESP32-CAM, ESP8266, Uno/Nano, relay, I2C LCD, buzzer, servo, HC-SR04, DHT, keypad, PIR |
| Sensor errors | float readers → `NAN` (`Integrall::valid(x)`), int readers → `-1` |
| Blocking policy | Nothing reachable from `loop()` waits > ~20 ms. Protocol µs waits (HC-SR04 trigger, HX711 clock, DHT bit-bang) allowed. |
| Library shape | Header-only; module flags via `#define` before include (Arduino IDE) or `build_flags` (PlatformIO, multi-file) |

## 3. Repository map (new/changed)

```
firmware/Integrall/src/
  Integrall.h              facade (System), all inline; selfTest(), MQTT wiring, built-in commands
  config/IntegrallConfig.h module flags, INTEGRALL_NETWORK_AVAILABLE, Integrall::valid()
  core/DeviceManager.h     async WiFi + HTTP(S) client (header-only; .cpp deleted)
  core/Util.h              fmtFloat (AVR has no %f!), i2cPresent, shared I2C pins, Backoff
  modules/*.h              every module rewritten or fixed (§5); RelayModule.cpp deleted
  modules/MqttModule.h     NEW cloud link (espMqttClient)
firmware/Integrall/examples/
  CloudMQTT/               NEW device <-> cloud example
  HardwareCheck/           NEW guided PASS/FAIL bench test (Uno runs it in 2 groups)
  SmartFallDevice/ BasicRelay/ SerialOnlyTest/   rewritten (no delay(), no blocking)
firmware/ci/               compile matrix: build_all.sh + platformio.ini + sketches/
firmware/tests/            host unit tests (PlatformIO native + Unity), fake Arduino in tests/mocks/
firmware/sim/              Wokwi simulation: ParkingSensor / KeypadLock / WeatherStation
cloud/backend/             FastAPI app, Alembic, pytest suite, Dockerfile
cloud/dashboard/           React app, Dockerfile (nginx proxies /api + WebSocket)
cloud/mosquitto/           broker config + first-run dynsec init entrypoint
cloud/e2e/e2e_test.py      end-to-end test against a REAL broker + backend
cloud/e2e/fake_device.py   simulated device (test cloud + dashboard with no hardware)
cloud/docker-compose.yml   postgres + mosquitto + backend + dashboard
```

## 4. How to verify

```bash
export PATH="$HOME/.platformio-venv/bin:$PATH"       # pio lives in this venv

# 1. Compile matrix (~75 builds; examples + coverage sketches x 5 boards; 30-60 min cold)
firmware/ci/build_all.sh                  # filter: build_all.sh CloudMQTT ; ENVS="uno" build_all.sh
#    logs: firmware/ci/logs/<Sketch>__<env>.log. Fails on compile errors AND RAM > 100 %.

# 2. Host unit tests (seconds, no hardware)
cd firmware/tests && pio test -e native     # 41 tests

# 3. Simulation (virtual ESP32 + parts)
cd firmware/sim && pio run -e parking      # then VS Code "Wokwi Simulator": open sim/parking/diagram.json
#    headless: WOKWI_CLI_TOKEN=... wokwi-cli parking   (free token: wokwi.com/dashboard/ci)

# 4. Backend tests
cd cloud/backend && python -m venv .venv && .venv/bin/pip install -r requirements-dev.txt && .venv/bin/pytest -q   # 12

# 5. Whole stack + e2e
cd cloud && cp .env.example .env    # edit every secret
docker compose up -d --build        # dashboard :8080, API :8000/docs, MQTT :1883
python e2e/e2e_test.py --api http://localhost:8000 --mqtt-host localhost       # 16 checks
python e2e/fake_device.py --id INT_A1B2C3D4E5F6 --token <token> --host localhost
# with OpenRemote running and the backend's INTEGRALL_OR_* set (cloud/openremote/README.md):
OR_CLIENT_SECRET=... python e2e/openremote_e2e.py --api http://localhost:8000 --mqtt-host localhost --insecure   # 25 checks
```

Environment notes: PlatformIO needed `pip install intelhex` in its venv. Owner's
Arduino IDE has ESP32 core 3.3.7; `~/Arduino/libraries/Integrall` is a **stale
copy** that shadows the repo in the IDE: replace it with a symlink to
`firmware/Integrall`. Port 5173 on the owner's machine is taken by another app
("Waka Admin"); run the dashboard dev server elsewhere:
`INTEGRALL_API=http://localhost:8000 npx vite --port 5199`.

Local e2e without docker:
```bash
mosquitto_ctrl dynsec init /tmp/dynsec.json backend-admin admin-pass
# mosquitto.conf: allow_anonymous false / listener 18830 /
#   plugin /usr/lib/x86_64-linux-gnu/mosquitto_dynamic_security.so / plugin_opt_config_file /tmp/dynsec.json
cd cloud/backend && INTEGRALL_DATABASE_URL=sqlite+aiosqlite:///e2e.db INTEGRALL_JWT_SECRET=x \
  INTEGRALL_MQTT_PORT=18830 INTEGRALL_MQTT_USERNAME=backend-admin INTEGRALL_MQTT_PASSWORD=admin-pass \
  sh -c 'alembic upgrade head && uvicorn app.main:app --port 18000'
python cloud/e2e/e2e_test.py --api http://localhost:18000 --mqtt-host 127.0.0.1 --mqtt-port 18830
```

## 5. Audit findings (v1.1.0) and status

🔴 broken/unsafe · 🟠 blocking/wrong · 🟡 quality. ☑ = fixed + compiled (+ host-tested where noted)

### Core
- ☑🔴 A1 `System` methods not `inline` → multi-file link errors. Library is header-only now.
- ☑🔴 A2 sketch flags invisible to library .cpp → WiFi/TLS always linked. Relay-only ESP32: **938 KB → 273 KB flash**.
- ☑🟠 A3 WiFi connect busy-waited up to 10 s → async, back-off 2→60 s, `onConnectionChange()`.
- ☑🟠 A4 `handle()` didn't tick keypad/RGB/time/GPS/cellular/stepper/OLED/MQTT.
- ☑🟡 A5 relays 4→8 (AVR 4) · A6 alarm no longer overwrites relay timeout (`sirenMs`) · A7 · A8
- ☑🟠 A9 firmware talked to an HTTP backend that never existed → MQTT + real cloud.
- ☑🟡 framework silently wrote "WiFi Discon..." on the user's LCD → opt-in `lcdShowNetworkStatus(row)`.

### Modules
- ☑🔴 R1 relay on GPIO 0 impossible · R2 interlock semantics · R3 32-bit counters (*host-tested*)
- ☑🟠 L1 two prints on one LCD row erased each other · L2 presence + 0x27/0x3F auto-detect (*host-tested*)
- ☑🟠 found by tests: label+value re-printed each loop re-sent the row over I2C every loop → row fingerprint
- ☑🟠 O1 OLED pushed 1 KB per print → dirty flag, ≤20 fps · O2 `enableOLED(addr)`
- ☑🟡 K1 keypad key loss · I1 encoder interrupts · S1 servo ease from unknown angle
- ☑🟠 ST1 stepper blocked whole move → own non-blocking driver + STEP/DIR (*host-tested*)
- ☑🟠 SE1-SE10 sensors: DS18B20 async, CO2/flow/HC-SR04 via interrupts, BH1750 restart bug, SHT3x CRC+async,
  VL53L0X continuous, HX711 never waits, multi DHT/OneWire, I2C back-off, NAN (*host-tested*)
- ☑🔴 C1 SIM800 blocked ~17 s and never read replies → AT queue state machine, `onSMSResult(ok)` (*host-tested*)
- ☑🔴 W1-W7 web/WebSocket: per-connection state machines; WS desync; fake `wss://`; constant masks;
  CSRF (Origin check + `X-Integrall` header); opt-in CORS; Basic auth; HTML escaping
- ☑🔴 PowerModule `beginINA(addr)` passed the address as `TwoWire*` (crash), hidden by ESP32 `-fpermissive`; CI now strict
- ☑🔴 AVR `snprintf("%f")` prints "?" → `Util::fmtFloat()`
- ☑🟡 M1 AVR deepSleep no hidden delay · M2 GPS fed in handle() · M3 no silent RTC set
- ☐🟡 M4 calling an API whose optional library isn't installed → add `static_assert` hints
- ☑ `selfTest()`: I2C scan with part names + module presence → Serial + retained MQTT `health`
- ☑ AVR RAM: `print/println(F("..."))`, selfTest strings in flash, OneWire linked only when used
- ☑ Examples: personal phone number + stored XSS removed from SmartFallDevice; delays removed

### Cloud
- ☑ Backend tests 12/12; Alembic migration == models (`alembic check` clean)
- ☑ Real-broker e2e 16/16: live push, command ack, LWT offline, offline refusal, wrong password rejected,
  **broker ACL blocks a device publishing as / spying on another device**
- ☑ Dashboard: strict TS build; checked in a browser against live backend + fake device
  (found+fixed: chart scale mixing, ack-before-POST race, relay state not republished → firmware now republishes meta on any relay change)

## 6. MQTT contract (firmware <-> backend): FROZEN for v1.2

```
integrall/{id}/status      retained  "online" | "offline" (LWT)
integrall/{id}/meta        retained  {fw, chip, board?, ip, mac, modules[], relays[{index,name,on}], switches[{name,on}]?}  (re-sent on relay/switch change)
integrall/{id}/health      retained  {parts{name:bool}, i2c[], heap, uptime_s, rssi, missing, fw}
integrall/{id}/telemetry   QoS0      {ts? (epoch, only if NTP-synced), <key>: number|bool|string ...}
integrall/{id}/event       QoS1      {ts?, type, detail}
integrall/{id}/cmd         device subscribes  {id, action, args}
integrall/{id}/cmd/ack     QoS1      {id, ok, msg?, result?}
```
Built-ins: `ping, health, reboot, relay.set{index,on}, relay.toggle{index}, relay.get{index}, relay.all_off,
switch.set{name,on}, switch.toggle{name}, switch.get{name}, buzzer.beep{ms}, buzzer.on{freq}, buzzer.off,
servo.set{index,angle}, lcd.print{text,col,row}`; custom via `integrall.onCommand()`.
(S2 additions are backward compatible: `switches` is optional in meta.)
Device id = `INT_` + 12 hex (MAC) = MQTT username; password = token from the dashboard (shown once, stored only by the broker).
Code: firmware `src/modules/MqttModule.h` + `Integrall.h` (`_mqttConnected`/`_mqttBuiltin`); backend `cloud/backend/app/bridge.py`.

## 7. Status / what's left

Done: verification infrastructure, firmware fixes, MQTT + backend + dashboard.

1. ☐ **Owner runs on real hardware** (§8). Record results in §10.
2. ☐ GUIDE.md: update for API changes (CHANGELOG "Breaking"). README already corrected.
3. ☐ M4 helpful errors for missing optional libraries.
4. ☐ Production hardening: broker TLS (8883 listener, `mqtts://` in firmware), login rate-limiting,
   telemetry retention / TimescaleDB, HTTPS in front of dashboard, JWT in httpOnly cookie instead of sessionStorage.
5. ☐ Known limits (documented in code): ESP8266 **TLS** MQTT reconnect blocks during the TLS handshake (plain MQTT is async);
   ESP32-CAM stream has no auth; `httpGet()` etc. are synchronous by nature (bounded by `setHttpTimeout`, default 5 s).
6. ☐ Wokwi headless in CI (needs WOKWI_CLI_TOKEN). ☐ v1.3 OTA from dashboard.
7. ☑ **OpenRemote live check** done against OpenRemote 1.34.1: `cloud/e2e/openremote_e2e.py` 20/20 (see
   `cloud/openremote/README.md` "Verified"). ☐ Open decision (owner, deferred): keep OpenRemote, rebrand it as
   "Integrall Remote" through its theming config, or grow our own React dashboard. Advice given: don't fork
   OpenRemote (AGPL, huge upstream).

## 8. Real-hardware test plan (owner)

1. Arduino IDE: replace `~/Arduino/libraries/Integrall` with a symlink to `firmware/Integrall` (or use PlatformIO).
2. **Any board, nothing wired:** `SerialOnlyTest` → device ID + self-test every 10 s.
3. **Parts:** `HardwareCheck`, answer prompts with y/n/s. Uno: second run with `#define CHECK_GROUP_B` (servo + keypad).
   Pins for ESP32 / ESP8266 / Uno are at the top of the sketch.
4. **Cloud:** `docker compose up` in `cloud/`, sign up on :8080, "Add device" with the ID from step 2, flash `CloudMQTT`
   with WiFi, `mqtt://<PC-IP>:1883` and the token. Toggle the relay from the dashboard.
5. **ESP32-CAM:** `SmartCamera` (stream on :81). **SIM800L:** separate 4 V / 2 A supply or it resets.

## 9. Progress log
- S1: audit (§5); PlatformIO + CI. Baseline v1.1.0: 51/51 core-2/8266/Uno/cam compile, 18/18 core-3 FAIL (LDF tooling → `lib_ldf_mode = chain`).
- S1: all firmware fixes in §5; host tests 38/38; Wokwi targets build.
- S1: cloud backend + dashboard + compose; backend 12/12; real-broker e2e 16/16; dashboard verified in a browser.
- S1: HardwareCheck, flash-string printing, lazy OneWire linking. Final compile matrix → §10.
- S2: OpenRemote chosen as the UI (bridge approach). Added `app/openremote.py`, a `Bridge.listeners` hook,
  `devices.or_asset_id` (Alembic 0002), compose OR_* env, `tools/integrall_devices.py`, and `cloud/openremote/README.md`.
  Backend tests 21/21 (12 + 9 OpenRemote, against a fake). Not yet run against a real OpenRemote (§7 item 7).
- S2: live OpenRemote 1.34.1 run. Fixes: ThingAsset requires notes+location; `command` is type JSON; new
  attributes are created with their first value (OR drops older-stamped writes); readable OR error messages.
  Backend 22/22; `openremote_e2e.py` 20/20 (twice); `e2e_test.py` 16/16.
- S2: first project = **ButtonBuzzer** (button toggles buzzer; buzzer is a cloud switch). Added `buzzerOn/IsOn`,
  active-buzzer mode, `addCloudSwitch()` + `switch.*` built-ins (firmware), switch support in the OR sync
  (writable attribute, dynamic subscribe, revert on failure), `firmware/projects/ButtonBuzzer` (PlatformIO,
  git-ignored `include/secrets.h`). Host tests 41/41, backend 24/24, `openremote_e2e.py` 25/25,
  ButtonBuzzer builds on esp32dev / esp32dev_core3 / nodemcuv2 with 0 lib warnings.
- S2: website docs (`website/IntegrallV1/src/Documentation.tsx` + `src/docs/{ui,start,modules}.tsx`) rewritten for
  v1.2: install, core rules, first project, self-test, every module, cloud + OpenRemote, MQTT topics, config,
  testing, migration. tsc + eslint clean, vite build OK, checked in a browser (desktop + 390 px).
- S2 pending owner: real-board run of ButtonBuzzer. Exposing the broker on the LAN (port 1883) for the
  ESP32 needs the owner's OK (it was blocked when tried). The dev stack is in the scratchpad
  (`e2e-or/`: mosquitto 127.0.0.1:18830, backend :18000, OpenRemote docker project `openremote`).

## 10. Results

### Final compile matrix (session 1)
- **77/77 builds pass**: every example + coverage sketch on esp32dev (core 2), esp32dev_core3, esp32cam, nodemcuv2, uno.
- Library warnings: the last two (parking `snprintf` truncation, unused `report` lambda in selfTest) fixed afterwards
  and re-verified on ParkingSensor/BlinkNonBlocking → 0 library warnings.
- Uno RAM: AllModulesAVR (9 modules) 89.6 % "tight"; HardwareCheck A 82.9 %, B 54.6 %; ParkingSensor 68.8 %.
- Host tests 38/38 after the last change.

### Real hardware
(owner: paste HardwareCheck output per board here)

## 11. Notes for the next agent
- Owner writes informally; "no pooling" = no polling.
- Don't commit unless asked. Leave the owner's untracked dirs (`IOT-Smart-Waste-Sorting/`, `Smart_AIR_Mouse/`, `website/IntegrallV1/`) and root zips alone.
- Never `pkill -f <pattern>` where the pattern is in your own command line (kills your shell); use `pgrep -f "[x]yz"`.
- `pio ci --keep-build-dir` doesn't refresh the copied library; build_all.sh deletes `$bdir/lib` each build; keep that.
- Never use `%f` in library code (AVR); use `F()` for long text on AVR.
