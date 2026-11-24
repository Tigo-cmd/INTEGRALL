# Changelog

## 1.2.0 — 2026-11 (in progress)

### Breaking
- **Networking is opt-in**: WiFi/HTTP/WebServer APIs exist only with `#define INTEGRALL_ENABLE_WIFI`
  (or `INTEGRALL_ENABLE_MQTT`). Relay-only ESP32 builds shrink from 938 KB to 273 KB.
- **Sensor failures return `NAN`** (floats) or `-1` (ints) instead of a mix of -999 / -1 / 0.
  Check with `Integrall::valid(x)` or `isnan(x)`.
- Slow sensors are now **asynchronous**: `readDistance`, `readProbeTemp` (DS18B20), `readLux`,
  `readSHT3x`, `readCO2PWM`, `readWaterFlowLPM`, `readWeightRaw` return the latest value instantly
  (NAN until the first measurement completes). CO2/flow/HC-SR04 echo need interrupt-capable pins (Uno: D2/D3).
- `sendTelemetry()` publishes over MQTT (`INTEGRALL_ENABLE_MQTT`); the HTTP "backend" calls were removed
  (no server ever implemented them).
- `begin()` with WiFi returns immediately; connection completes in the background (`onConnectionChange()`).
- `cellularBegin()` returns immediately; `sendSMS()` queues and returns `bool`; result via `onSMSResult(cb)`.
- `stepperStep()/stepperMove()` no longer block; motion runs in `handle()` (`stepperIsMoving()`).
- `lcdPrint()` at column > 0 writes in place (column 0 still replaces the line).
- `enableIMU()` defaults to the board's I2C pins (was hard-coded 21/22, wrong on ESP8266).
- Web handler routes reject cross-site browser requests; pages from `htmlPage()` send `X-Integrall: 1`
  automatically. Use `webAllowCrossSite()` / `webEnableCORS()` to opt out.
- No longer writes "WiFi Discon..." to your LCD; opt in with `lcdShowNetworkStatus(row)`.

### Added
- MQTT cloud link (`INTEGRALL_ENABLE_MQTT`), built-in cloud commands, `onCommand()`, `sendEvent()`.
- `selfTest()` — I2C scan + per-module "is the part connected" report (also published as `health`).
- Integrall Cloud (`cloud/`): FastAPI + Postgres + Mosquitto backend and docker-compose.
- **OpenRemote as the UI**: the backend mirrors every device as an OpenRemote asset (telemetry with history,
  health, events) and turns OpenRemote writes into commands (`relay<N>` switches, `command` attribute).
  See `cloud/openremote/README.md`. The React dashboard is retired (compose profile `legacy-dashboard`).
  Devices are registered with `cloud/tools/integrall_devices.py`.
- Examples: `CloudMQTT`, `HardwareCheck`, `ButtonBuzzer` (first project; PlatformIO project in `firmware/projects/ButtonBuzzer`).
- `addCloudSwitch(name, get, set)`: any on/off output becomes a dashboard switch (meta `switches[]`,
  commands `switch.set/toggle/get`); state changes republished automatically.
- Buzzer: `buzzerOn()/buzzerIsOn()` (latched), active-buzzer modules (`enableBuzzer(pin, true)`),
  cloud commands `buzzer.on`/`buzzer.off`.
- Website documentation rewritten for v1.2 (`website/IntegrallV1/src/docs/`).
- Stepper STEP/DIR drivers (A4988/DRV8825), multiple DHT/DS18B20 sensors, encoder interrupts,
  `print(F(...))`, `lcdPrintValue()`, `enableOLED(addr)`, `setI2CPins()`, `buzzerIsActive()`, WebSocket `wss://`.
- Verification: compile matrix (`firmware/ci`), host unit tests (`firmware/tests`), Wokwi simulation (`firmware/sim`).

### Fixed
- Relay on GPIO 0; interlock dead code; INA219 crash (address passed as `TwoWire*`); BH1750 never
  finishing a measurement; AVR `%f` printing "?"; WebSocket stream corruption on large frames;
  `wss://` silently unencrypted; SIM800 "SMS sent" logged even on failure; multi-file link errors;
  key presses lost between `keypadCapture()` and `keypadSubmitted()`; dozens of blocking `delay()`s.
