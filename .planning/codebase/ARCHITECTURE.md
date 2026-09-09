<!-- refreshed: 2026-09-09 -->
# Architecture

**Analysis Date:** 2026-09-09

## System Overview

INTEGRALL is a **multi-component IoT ecosystem** consisting of:
1. **Core Firmware Framework** — C++ library for ESP32/Arduino hardware abstraction
2. **Web Platform** — React-based landing page, documentation, and dashboards
3. **Full-Stack Applications** — Complete IoT solutions built on the framework

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                          User Applications                                   │
│  Weather Station │ Smart Waste Sorting │ Smart AIR Mouse │ Examples         │
│  (React App)     │ (React + Python)    │ (Arduino)        │ (Arduino)        │
└──────────┬───────┴────────────┬────────┴────────┬────────┴────────┬─────────┘
           │                    │                 │                │
           ▼                    ▼                 ▼                ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                        INTEGRALL Core Framework                              │
│                    `firmware/Integrall/src/`                               │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │ Integrall::System — Main entry point (Integrall.h)                    │ │
│  │ - Device lifecycle: begin() → handle()                                │ │
│  │ - Module registration and lifecycle management                        │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │ Core Services                                                          │ │
│  │ • DeviceManager (`core/DeviceManager.h/.cpp`) — WiFi, registration  │ │
│  │ • Logger (`core/Logger.h`) — Debug output routing                     │ │
│  │ • IntegrallConfig (`config/IntegrallConfig.h`) — Compile flags       │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │ 17+ Hardware Modules (modules/)                                        │ │
│  │ Input:      InputModule, KeypadModule, SensorModule                   │ │
│  │ Output:     RelayModule, ServoModule, StepperModule, BuzzerModule    │ │
│  │ Display:    LCDModule, OLEDModule                                     │ │
│  │ Sensing:    SensorModule (DHT, BME280, distance)                     │ │
│  │ Advanced:   CameraModule, AudioModule, CommunicationModule            │ │
│  │ Power:      PowerModule, BlinkerModule, RGBModule                     │ │
│  │ Storage:    StorageModule, TimeModule, CellularModule                 │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
           │                    │                 │                │
           ▼                    ▼                 ▼                ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Backend Services & External APIs                          │
│  Device Registration │ Data Telemetry │ Command Control │ LLM Vision       │
│  (REST API)          │ (HTTP POST)     │ (HTTP Pull)     │ (Groq Vision)    │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Component Responsibilities

| Component | Responsibility | File |
|-----------|----------------|------|
| **Integrall::System** | Main framework entry point, lifecycle orchestration | `firmware/Integrall/src/Integrall.h` |
| **DeviceManager** | WiFi connection, backend registration, device state machine | `firmware/Integrall/src/core/DeviceManager.h/.cpp` |
| **Logger** | Debug/telemetry output routing (serial, MQTT optional) | `firmware/Integrall/src/core/Logger.h` |
| **Hardware Modules** | Individual abstractions for relays, sensors, displays, actuators | `firmware/Integrall/src/modules/*.h` |
| **Weather Station App** | React+TypeScript dashboard for sensor telemetry visualization | `weather_station/src/` |
| **Website** | React landing page, documentation, interactive guides | `website/IntegrallV1/src/` |
| **Waste Sorting Backend** | Python Flask server for image classification via Groq LLM | `IOT-Smart-Waste-Sorting/serverside/app.py` |
| **Waste Sorting Client** | React frontend for waste management dashboard | `IOT-Smart-Waste-Sorting/client/project/src/` |

## Pattern Overview

**Overall:** **Modular Hardware Abstraction Layer with Compile-Time Configuration**

**Key Characteristics:**
- **Compile-time conditional compilation** — Modules enabled via `#define` flags, unused code compiles to zero bytes
- **Non-blocking event loop** — All hardware I/O uses polling/interrupts, no blocking delays except user code
- **Centralized lifecycle** — Single `Integrall::System` manages all modules; user calls `begin()` then repeatedly calls `handle()`
- **Backend-agnostic** — Can operate offline (Arduino) or with WiFi registration to REST API
- **Layered abstraction** — Hardware drivers → Module wrappers → User API, user code never touches raw GPIO

## Layers

**Application Layer:**
- Purpose: User sketches, React dashboards, Flask backends
- Location: `firmware/Integrall/examples/`, `weather_station/src/`, `website/IntegrallV1/src/`, `IOT-Smart-Waste-Sorting/`
- Contains: `.ino` sketches, `.tsx` React components, `.py` Flask routes
- Depends on: Framework APIs (C++ for firmware, REST/Fetch for web apps)
- Used by: End users, IoT devices, web browsers

**Framework Layer (Integrall.h):**
- Purpose: Public API surface for all modules and system functions
- Location: `firmware/Integrall/src/Integrall.h`
- Contains: Class definitions, inline method implementations, configuration documentation
- Depends on: Core services (DeviceManager, Logger)
- Used by: User sketches

**Core Services Layer:**
- Purpose: Device lifecycle, networking, logging infrastructure
- Location: `firmware/Integrall/src/core/`
- Contains: `DeviceManager.h/.cpp` (WiFi state machine, registration), `Logger.h` (output routing)
- Depends on: Platform-specific headers (ESP32 WiFi, Arduino Serial)
- Used by: Framework layer, modules

**Module Implementation Layer:**
- Purpose: Hardware-specific abstractions with safety logic
- Location: `firmware/Integrall/src/modules/*.h`
- Contains: Relay control with timeouts, sensor reading with averaging, LCD/OLED drivers
- Depends on: Arduino/ESP32 HAL, core Logger
- Used by: Framework layer via Integrall::System

**Backend Service Layer (Web):**
- Purpose: API endpoints for device telemetry, control, and intelligence
- Location: `IOT-Smart-Waste-Sorting/serverside/app.py` (Flask), `weather_station/` endpoints
- Contains: REST routes, LLM vision integration, file upload handlers
- Depends on: Flask, Groq SDK, file I/O
- Used by: Firmware devices (HTTP POST), React frontends (Fetch/Axios)

**Frontend Layer (React):**
- Purpose: Interactive dashboards and documentation
- Location: `website/IntegrallV1/src/`, `weather_station/src/`, `IOT-Smart-Waste-Sorting/client/project/src/`
- Contains: React components, TypeScript types, CSS (Tailwind)
- Depends on: React DOM, external APIs (Supabase optional, Groq via backend)
- Used by: Web browsers

## Data Flow

### Primary Request Path (Firmware Device → Backend)

1. **Setup Phase** (`firmware/Integrall/examples/*.ino`)
   - User defines module flags and includes `Integrall.h`
   - `Integrall::System integrall` instantiated
   - `integrall.begin(config)` called in Arduino `setup()`
   - DeviceManager connects to WiFi and registers with backend

2. **Polling Phase** (`firmware/Integrall/src/Integrall.h` → `loop()` callback)
   - `integrall.handle()` called repeatedly in Arduino `loop()`
   - DeviceManager checks for commands from backend
   - Each enabled module polls its hardware (sensors, buttons, etc.)
   - Non-blocking callbacks triggered on state changes

3. **Data Collection** (Module layer)
   - Sensors sampled → `SensorModule.read()` returns averaged value
   - Relay states checked → `RelayModule.handle()` enforces safety timeouts
   - Display updated → `LCDModule.print()` queued for next refresh

4. **Telemetry Transmission** (`DeviceManager::sendTelemetry()`)
   - Collected data serialized to JSON via `ArduinoJson`
   - HTTP POST to `backend_url/api/telemetry` with device ID + API key
   - Backend logs to database or streams to frontend

5. **Frontend Visualization**
   - Weather station React app: `App.tsx` polls `localhost:5000/api/live` every 4 seconds
   - Waste sorting dashboard: REST fetch from Flask backend
   - Components (`SensorMatrix`, `Charts`, `TelemetryStream`) render real-time data

### Weather Station Polling Flow (Frontend → Backend)

1. **Entry:** `weather_station/src/App.tsx` → `useEffect`
2. **Fetch:** `fetch('http://localhost:5000/api/live')`
3. **Parse:** Response mapped to `WeatherData` interface (`weather_station/src/types.ts`)
4. **State Update:** Historical data accumulated (max 50 entries), telemetry logged (max 100 entries)
5. **Render:** Components display current metrics + rate of change calculations
6. **Repeat:** `setInterval(fetchWeatherData, 4000)`

### Waste Sorting Image Classification Flow

1. **Client Upload:** User selects image in React dashboard
2. **Server Receipt:** Flask route `/api/classify` receives multipart file
3. **Vision LLM:** `analyze_vision_with_groq()` encodes image to base64 → `groq_client.chat.completions.create()` with vision model
4. **Classification:** LLM returns structured JSON (waste type, confidence)
5. **Database Update:** Detection recorded in-memory db, sent back to client
6. **Client Display:** React dashboard updates with classification result

**State Management:**
- **Firmware:** Module state stored in `Integrall::System` instance, persistent across `handle()` calls
- **Weather Station:** React `useState` hooks maintain historical data, system status, telemetry logs
- **Waste Sorting:** In-memory db dict (devices, detections, alerts); Flask persists uploads to disk

## Key Abstractions

**Integrall::System:**
- Purpose: Unified entry point managing all hardware modules
- Examples: `firmware/Integrall/examples/BasicRelay.ino`, `firmware/Integrall/examples/FullDemo.ino`
- Pattern: **Facade pattern** — hides module complexity behind simple `begin()` and `handle()` methods

**Module Base Design (Mixin Pattern):**
- Purpose: Each hardware component (Relay, Sensor, LCD) follows consistent API
- Examples: `RelayModule.h`, `SensorModule.h`, `LCDModule.h`
- Pattern: Header-only implementations with compile-time feature flags

**WeatherData Interface:**
- Purpose: Type-safe data contract between firmware telemetry and React UI
- Examples: `weather_station/src/types.ts` defines `WeatherData`, `SystemStatus`, `TelemetryLog`
- Pattern: TypeScript interfaces as runtime documentation

**DeviceConfig Structure:**
- Purpose: Centralized device configuration (WiFi, backend URL, API key)
- Examples: `firmware/Integrall/src/core/DeviceManager.h` → `struct DeviceConfig`
- Pattern: Builder-like configuration object passed to `begin(config)`

## Entry Points

**Firmware Entry Point:**
- Location: `firmware/Integrall/examples/*.ino`
- Triggers: Arduino IDE upload or PlatformIO deploy
- Responsibilities:
  1. Define module flags (`#define INTEGRALL_ENABLE_RELAY`, etc.)
  2. Include main header (`#include <Integrall.h>`)
  3. Instantiate system (`Integrall::System integrall`)
  4. Initialize in `setup()` with `integrall.begin(config)`
  5. Call `integrall.handle()` in `loop()`

**Weather Station UI Entry Point:**
- Location: `weather_station/src/main.tsx`
- Triggers: Browser loads `http://localhost:5173` (dev) or built URL
- Responsibilities:
  1. Mount React app to DOM root
  2. Render `<App />` component in StrictMode
  3. App.tsx initiates polling, manages state, renders dashboard

**Website Entry Point:**
- Location: `website/IntegrallV1/src/main.tsx`
- Triggers: Browser loads landing page
- Responsibilities:
  1. Mount React app
  2. Render `<App />` which switches between LandingPage and Documentation views

**Waste Sorting Backend Entry Point:**
- Location: `IOT-Smart-Waste-Sorting/serverside/app.py` → `if __name__ == '__main__'`
- Triggers: `python app.py` or gunicorn/Flask development server
- Responsibilities:
  1. Initialize Flask app + CORS
  2. Load Groq API key from env
  3. Define routes for device management, image classification, detection retrieval
  4. Serve uploads directory for image viewing

## Architectural Constraints

- **Threading:** Single-threaded event loop (Arduino/ESP32). No OS threads. Firmware runs `loop()` continuously without blocking.
- **Global state:** `Integrall::System` instance is user-created singleton; module state lives within it. No module-level statics.
- **Circular imports:** Avoided via **header-only modules**. Integrall.h includes all module headers; modules never include each other.
- **Offline-first:** Firmware can run without WiFi (offline-only mode). Network failures do not crash device.
- **Memory constraints:** Compile-time module selection ensures zero overhead for disabled features. Firmware targets ESP32 (4MB+ flash) and Arduino UNO (32KB flash).
- **Latency:** Non-blocking I/O required. Sensors use polling; actuators use interrupt-driven state machines with timeouts.
- **Modularity:** Each module is independent. Users can enable/disable via compile flags without editing module code.

## Anti-Patterns

### Long-Running Blocking Calls in loop()

**What happens:** User code calls `delay()`, blocking HTTP requests, or long serial reads in Arduino loop
**Why it's wrong:** Freezes all module polling. Relay timeouts don't fire, sensor reads skip cycles, device appears unresponsive.
**Do this instead:** Use non-blocking patterns. For HTTP: use `WiFiClient` with timeout + manual reading. For timing: track `millis()` and compare. See `firmware/Integrall/src/Integrall.h` → `handle()` implementation.

### Direct GPIO Access Bypassing Framework

**What happens:** User code calls `digitalWrite(pin, HIGH)` directly instead of `integrall.setRelay(id, true)`
**Why it's wrong:** Bypasses safety timeouts, interlock groups, and logging. Device can remain on indefinitely or violate safety constraints.
**Do this instead:** Always use module APIs. Relays: `integrall.setRelay()`. Sensors: `integrall.readSensor()`. Framework enforces safety.

### Mixing Offline and Online Logic

**What happens:** User code assumes backend is always available and crashes if network fails
**Why it's wrong:** Devices in the field lose WiFi. INTEGRALL_BACKEND_ENABLED=0 compilation will fail.
**Do this instead:** Write conditional code: `if (integrall.isOnline()) { /* fetch from backend */ } else { /* use local state */ }`. See `firmware/Integrall/examples/ArduinoOfflineTest.ino`.

### Module Configuration After begin()

**What happens:** User calls `integrall.enableRelay(5)` after `integrall.begin(config)`
**Why it's wrong:** Module state is already initialized. Calling enable() late may cause dangling module references or resource leaks.
**Do this instead:** Call all `enable*()` methods **immediately** after `begin()`, before `loop()` starts. See `firmware/Integrall/examples/FullDemo.ino` for correct pattern.

### Relying on Arduino delay() for Sensor Timing

**What happens:** User tries to throttle sensor reads with `delay(1000)` in the module
**Why it's wrong:** Blocks all other modules. Firmware should poll at controller cadence via `integrall.handle()`, not sleep.
**Do this instead:** Module internally tracks `millis()` and skips processing if insufficient time has passed. SensorModule uses this approach (see `modules/SensorModule.h`).

## Error Handling

**Strategy:** Non-crashing fault tolerance with state fallback

**Patterns:**
- **WiFi Failures:** DeviceManager enters `DISCONNECTED` state, continues polling hardware in offline mode, retries WiFi connection periodically
- **Sensor Read Failures:** Return last valid reading; log error via Logger
- **Relay Safety Timeout Exceeded:** Force relay OFF, log safety violation
- **Backend Registration Timeout:** Log error, continue in offline mode; retry on next handle() cycle
- **Invalid Module ID:** Return false; do not crash. User checks return value (see `firmware/Integrall/examples/ArduinoOfflineTest.ino`)

## Cross-Cutting Concerns

**Logging:** 
- Framework: Serial output (debug level configurable via `INTEGRALL_DEBUG_LEVEL` flag)
- Logger.h provides `DEBUG`, `INFO`, `WARN`, `ERROR` macros
- Optional MQTT logging for production (requires `INTEGRALL_ENABLE_MQTT`)

**Validation:**
- WiFi credentials validated before connection attempt
- Relay GPIO pins checked for valid range (0-39 for ESP32)
- Sensor readings filtered via moving average (configurable window)
- Keypad input sanitized before password comparison

**Authentication:**
- Firmware: API key sent in HTTP headers for backend registration + telemetry
- Web: No built-in auth (optional Supabase integration via `@supabase/supabase-js` in weather_station)
- Waste Sorting: Groq API key loaded from env (server-side only, never exposed to browser)

---

*Architecture analysis: 2026-09-09*
