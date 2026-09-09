# Codebase Structure

**Analysis Date:** 2026-09-09

## Directory Layout

```
/home/tigo/Desktop/Myprojects/INTEGRALL/
├── firmware/                          # Core IoT Framework (C++ Arduino Library)
│   ├── platformio.ini                 # PlatformIO project config
│   ├── library.json                   # Arduino Library manifest
│   └── Integrall/                     # Main framework package
│       ├── library.properties         # Arduino library metadata
│       ├── keywords.txt               # Arduino syntax highlighting
│       ├── src/
│       │   ├── Integrall.h            # Main entry point (1707 lines)
│       │   ├── config/
│       │   │   └── IntegrallConfig.h  # Compile-time configuration flags
│       │   ├── core/
│       │   │   ├── DeviceManager.h    # WiFi, device registration, state machine
│       │   │   ├── DeviceManager.cpp  # Implementation (404 lines)
│       │   │   └── Logger.h           # Debug output routing
│       │   └── modules/               # 17+ hardware abstraction modules
│       │       ├── RelayModule.h      # GPIO relay control with timeouts
│       │       ├── RelayModule.cpp    # Relay logic implementation
│       │       ├── SensorModule.h     # Multi-sensor abstraction (DHT, BME280, HC-SR04)
│       │       ├── InputModule.h      # Button/switch input debouncing
│       │       ├── LCDModule.h        # 16x2/20x4 LCD display control
│       │       ├── OLEDModule.h       # OLED display (I2C SSD1306)
│       │       ├── ServoModule.h      # PWM servo control with easing
│       │       ├── StepperModule.h    # Stepper motor control
│       │       ├── KeypadModule.h     # Matrix keypad input + password auth
│       │       ├── BuzzerModule.h     # Piezo buzzer control
│       │       ├── BlinkerModule.h    # LED blinking patterns
│       │       ├── RGBModule.h        # RGB LED PWM control
│       │       ├── PowerModule.h      # INA219 power monitoring
│       │       ├── AudioModule.h      # MP3 audio playback
│       │       ├── CameraModule.h     # ESP32-CAM MJPEG streaming
│       │       ├── CommunicationModule.h # UART/wireless protocols
│       │       ├── CellularModule.h   # 4G/LTE connectivity
│       │       ├── StorageModule.h    # SD card abstraction
│       │       ├── TimeModule.h       # RTC + NTP time sync
│       │       └── camera/            # Camera helper files (MJPEG streaming)
│       └── examples/                  # Ready-to-flash project templates
│           ├── BasicRelay.ino         # Simple relay control
│           ├── SmartSwitch.ino        # Relay + sensor demo
│           ├── WeatherStation.ino     # Multi-sensor monitoring
│           ├── KeypadLock.ino         # Security keypad + relay
│           ├── SmartCamera.ino        # ESP32-CAM streaming
│           ├── FullDemo.ino           # All modules showcase
│           ├── DistanceAlerts.ino     # HC-SR04 proximity sensor
│           └── ArduinoOfflineTest.ino # Offline-only mode test
│
├── website/                           # Documentation & Landing Page
│   ├── IntegrallV1/                  # Main React project
│   │   ├── package.json              # Dependencies (React 19, Framer Motion, Tailwind)
│   │   ├── tsconfig.json             # TypeScript config
│   │   ├── vite.config.ts            # Vite build config
│   │   ├── eslint.config.js          # ESLint rules
│   │   ├── src/
│   │   │   ├── main.tsx              # React entry point
│   │   │   ├── App.tsx               # App router/state
│   │   │   ├── LandingPage.tsx       # Marketing homepage
│   │   │   ├── Documentation.tsx     # Full framework docs (125KB)
│   │   │   ├── index.css             # Global styles
│   │   │   ├── App.css               # App-specific styles
│   │   │   ├── lib/
│   │   │   │   └── utils.ts          # Helper functions
│   │   │   ├── assets/               # Static images, logos
│   │   │   └── components/
│   │   │       ├── layout/
│   │   │       │   ├── PlatformsSection.tsx       # Hardware platforms showcase
│   │   │       │   ├── ProjectsSection.tsx        # Example projects
│   │   │       │   ├── AsciiFooter.tsx            # ASCII art footer
│   │   │       │   ├── ScaleSection.tsx           # Scalability messaging
│   │   │       │   ├── CloudSection.tsx           # Cloud integration info
│   │   │       │   ├── FeaturesSection.tsx        # Features list
│   │   │       │   ├── ModulesSection.tsx         # Module showcase
│   │   │       │   ├── InstallSection.tsx         # Installation guide
│   │   │       │   ├── StatsSection.tsx           # Statistics/metrics
│   │   │       │   └── CtaBannerAndFooter.tsx    # Call-to-action
│   │   │       └── ui/               # Reusable UI components
│   │   │           ├── button.tsx
│   │   │           ├── animated-heading.tsx
│   │   │           ├── progressive-blur.tsx
│   │   │           ├── infinite-slider.tsx
│   │   │           └── toast-provider.tsx
│   │   ├── index.html                # HTML template
│   │   └── tailwind.config.js         # Tailwind CSS config
│   └── css/
│   └── js/                            # Legacy HTML/CSS/JS (optional)
│
├── weather_station/                   # React + TypeScript Monitoring Dashboard
│   ├── package.json                  # Dependencies (React 18, Recharts, Supabase)
│   ├── tsconfig.json                 # TypeScript config
│   ├── tsconfig.app.json             # App-only TypeScript config
│   ├── vite.config.ts                # Vite dev server config (port 5173)
│   ├── eslint.config.js              # ESLint rules
│   ├── src/
│   │   ├── main.tsx                  # React entry point
│   │   ├── App.tsx                   # Main dashboard (polls localhost:5000/api/live)
│   │   ├── index.css                 # Global styles (Tailwind)
│   │   ├── types.ts                  # TypeScript interfaces (WeatherData, SystemStatus)
│   │   ├── vite-env.d.ts             # Vite type definitions
│   │   └── components/
│   │       ├── SystemHealthHeader.tsx    # Device status display (online/uptime/lastSync)
│   │       ├── SensorMatrix.tsx          # 5-column sensor metrics grid
│   │       ├── Charts.tsx                # Historical data chart (Recharts LineChart)
│   │       └── TelemetryStream.tsx       # Live log of sensor readings
│   ├── index.html                    # HTML template
│   └── tailwind.config.js             # Tailwind CSS config
│
├── IOT-Smart-Waste-Sorting/          # Full-Stack Waste Classification System
│   ├── client/                        # React Frontend
│   │   └── project/
│   │       ├── package.json           # Dependencies (React, Vite)
│   │       ├── tsconfig.json          # TypeScript config
│   │       ├── vite.config.ts         # Vite config
│   │       ├── src/
│   │       │   ├── main.tsx           # React entry point
│   │       │   ├── App.tsx            # Main app component
│   │       │   ├── components/        # UI components for waste sorting
│   │       │   ├── assets/            # Images, logos
│   │       │   └── ...
│   │       └── index.html             # HTML template
│   ├── serverside/                    # Python Flask Backend
│   │   ├── app.py                    # Main Flask app (8.2KB)
│   │   │                             # Routes: /api/devices, /api/classify, /api/detections
│   │   │                             # Uses Groq Vision API for waste classification
│   │   ├── requirements.txt           # Python dependencies (Flask, Groq, CORS)
│   │   ├── test_inference.py          # Test script for vision LLM
│   │   ├── .env                       # Environment config (GROQ_API_KEY)
│   │   ├── uploads/                   # Uploaded images storage
│   │   ├── yolov8n.pt                 # YOLOv8 nano model (optional)
│   │   └── yolov8s.pt                 # YOLOv8 small model (optional)
│   └── Firmware/                      # Arduino firmware for waste bin device
│       └── ... (similar to Integrall framework)
│
├── Smart_AIR_Mouse/                   # Arduino-based Gesture Input Device
│   └── AIR_MOUSE.ino                 # Standalone Arduino sketch for air mouse
│
├── docs/                              # Documentation directory
│   └── (various .md files)
│
├── .planning/                         # Planning and analysis documents
│   └── codebase/                      # Generated codebase analysis
│       ├── ARCHITECTURE.md            # System architecture and layers
│       ├── STRUCTURE.md               # This file
│       └── (other analysis docs)
│
├── README.md                          # Project overview
├── GUIDE.md                           # Developer guide
├── DEPENDENCIES.md                    # Dependency documentation
├── PRD_LANDING_PAGE.md               # Product requirements
├── PROMPT_FRONTEND_DASHBOARD.md      # Frontend specification
├── .gitignore                         # Git ignore rules
├── code.cpp                           # Legacy C++ code snippet
├── smartglasses.cpp                   # Legacy project
├── test.py                            # Test script
├── simulate_esp32.py                  # ESP32 simulator
├── weather_backend.py                 # Backend for weather station
└── TestCode1.ino                      # Test Arduino sketch

```

## Directory Purposes

**`firmware/`**
- Purpose: Core IoT framework library and examples
- Contains: C++ Arduino library with modular hardware abstractions
- Key files: `firmware/Integrall/src/Integrall.h` (main entry), `firmware/platformio.ini` (build config)
- Built for: ESP32, ESP8266, Arduino Uno (with feature limitations)

**`firmware/Integrall/src/`**
- Purpose: Framework implementation
- Contains: Core services, module definitions, configuration constants
- Key files: `Integrall.h`, `core/DeviceManager.h`, `modules/*.h`

**`firmware/Integrall/examples/`**
- Purpose: Ready-to-flash project templates
- Contains: Complete `.ino` sketches demonstrating module usage
- Key files: `BasicRelay.ino`, `WeatherStation.ino`, `FullDemo.ino`, `KeypadLock.ino`
- How to use: Open in Arduino IDE, select board (ESP32-DEV, Arduino Uno, etc.), flash to device

**`website/IntegrallV1/src/`**
- Purpose: Marketing website and documentation portal
- Contains: React components for landing page and full API docs
- Key files: `LandingPage.tsx` (home), `Documentation.tsx` (full docs), `components/layout/*` (sections)
- Entry point: `main.tsx` → `<App />`

**`weather_station/src/`**
- Purpose: Real-time sensor monitoring dashboard
- Contains: React components for telemetry visualization
- Key files: `App.tsx` (main polling logic), `components/SystemHealthHeader.tsx`, `components/Charts.tsx`
- Entry point: `main.tsx` → `<App />`
- Data source: Polls `http://localhost:5000/api/live` every 4 seconds

**`IOT-Smart-Waste-Sorting/serverside/`**
- Purpose: Backend API for waste classification
- Contains: Flask routes, Groq LLM integration, file upload handling
- Key files: `app.py` (main server, ~8.2KB with routes)
- Routes:
  - `GET /api/devices` — List registered bins
  - `POST /api/classify` — Upload image, get waste classification
  - `GET /api/detections` — Retrieve all classifications
- Depends on: Groq API key (env var `GROQ_API_KEY`)

**`IOT-Smart-Waste-Sorting/client/project/src/`**
- Purpose: Frontend for waste management dashboard
- Contains: React components for image upload and result display
- Key files: `App.tsx`, `components/`

**`docs/`**
- Purpose: Documentation files
- Contains: Various markdown files (implementation details, API reference)

**`.planning/codebase/`**
- Purpose: Generated codebase analysis documents
- Contains: ARCHITECTURE.md, STRUCTURE.md, CONVENTIONS.md, TESTING.md, CONCERNS.md
- Generated by: `/gsd-map-codebase` skill

## Key File Locations

**Entry Points:**

| Component | Entry Point | Language | Purpose |
|-----------|-------------|----------|---------|
| Firmware | `firmware/Integrall/examples/BasicRelay.ino` | Arduino C++ | Template sketch to flash |
| Firmware Library | `firmware/Integrall/src/Integrall.h` | C++ Header | Only file users include |
| Weather App | `weather_station/src/main.tsx` | TypeScript/React | React DOM mount |
| Website | `website/IntegrallV1/src/main.tsx` | TypeScript/React | React DOM mount |
| Waste Backend | `IOT-Smart-Waste-Sorting/serverside/app.py` | Python/Flask | Flask app server |
| Waste Client | `IOT-Smart-Waste-Sorting/client/project/src/main.tsx` | TypeScript/React | React DOM mount |

**Configuration:**

| File | Purpose | Scope |
|------|---------|-------|
| `firmware/platformio.ini` | PlatformIO build targets (ESP32, Arduino) | Firmware builds |
| `firmware/Integrall/src/config/IntegrallConfig.h` | Compile-time feature flags | Framework compilation |
| `weather_station/vite.config.ts` | Dev server port, build output | Weather app dev |
| `website/IntegrallV1/vite.config.ts` | Dev server port, build output | Website dev |
| `IOT-Smart-Waste-Sorting/serverside/.env` | GROQ_API_KEY | Backend API access |
| `.planning/codebase/ARCHITECTURE.md` | System design reference | All components |

**Core Logic:**

| File | Purpose | Key Functions/Classes |
|------|---------|----------------------|
| `firmware/Integrall/src/Integrall.h` | Main API surface | `class Integrall::System` |
| `firmware/Integrall/src/core/DeviceManager.cpp` | WiFi + registration | `DeviceManager::connect()`, `::register()`, `::handle()` |
| `firmware/Integrall/src/modules/SensorModule.h` | Sensor reading | `readTemperature()`, `readHumidity()` (with averaging) |
| `firmware/Integrall/src/modules/RelayModule.cpp` | Relay control | `setRelay()`, `handle()` (timeout enforcement) |
| `weather_station/src/App.tsx` | Polling + state | `fetchWeatherData()`, `calculateRateOfChange()` |
| `IOT-Smart-Waste-Sorting/serverside/app.py` | Vision AI | `analyze_vision_with_groq()`, `/api/classify` route |

**Testing:**

| File | Purpose |
|------|---------|
| `firmware/Integrall/examples/ArduinoOfflineTest.ino` | Offline mode test |
| `IOT-Smart-Waste-Sorting/serverside/test_inference.py` | Vision model test |
| `test.py`, `test_buzzer.cpp`, `test_relay.cpp` | Ad-hoc test files |

## Naming Conventions

**Files:**

- **C++ Headers:** `PascalCaseModule.h` (e.g., `RelayModule.h`, `SensorModule.h`)
- **C++ Source:** `PascalCaseModule.cpp` (e.g., `DeviceManager.cpp`)
- **Arduino Sketches:** `PascalCaseName.ino` (e.g., `BasicRelay.ino`, `KeypadLock.ino`)
- **React Components:** `PascalCaseName.tsx` (e.g., `App.tsx`, `SystemHealthHeader.tsx`)
- **React Hooks/Utils:** `camelCaseName.ts` (e.g., `utils.ts`)
- **Config Files:** lowercase with dot notation (e.g., `vite.config.ts`, `postcss.config.js`)
- **Types/Interfaces:** `PascalCaseName.ts` or inline in `.tsx` (e.g., `types.ts`)
- **Environment:** `.env` (main), `.env.local` (untracked, secrets)

**Directories:**

- **C++ Modules:** `modules/` (all hardware abstractions)
- **Core Services:** `core/` (DeviceManager, Logger)
- **Configuration:** `config/` (compile-time flags)
- **React Components:** `components/` (nested by feature)
- **React Utils:** `lib/` (helpers, utilities)
- **Static Assets:** `assets/` (images, fonts)
- **Layouts:** `components/layout/` (page sections)
- **UI Primitives:** `components/ui/` (buttons, inputs)

**TypeScript/JavaScript:**

- **Interfaces:** `PascalCase` (e.g., `WeatherData`, `SystemStatus`)
- **Functions:** `camelCase` (e.g., `fetchWeatherData`, `calculateRateOfChange`)
- **Constants:** `UPPER_SNAKE_CASE` (e.g., `POLLING_INTERVAL`, `MAX_HISTORY`)
- **Classes:** `PascalCase` (e.g., `Integrall::System`)
- **Namespaces:** `PascalCase` (e.g., `namespace Integrall { }`)

**C++:**

- **Classes:** `PascalCase` (e.g., `RelayModule`, `DeviceManager`)
- **Structs:** `PascalCase` (e.g., `DeviceConfig`, `WeatherData`)
- **Methods:** `camelCase` (e.g., `enableRelay()`, `readSensor()`)
- **Macros:** `UPPER_SNAKE_CASE` (e.g., `INTEGRALL_ENABLE_RELAY`)
- **Enums:** `PascalCase` (e.g., `DeviceState`, `SensorType`)

## Where to Add New Code

**New Firmware Module (e.g., MotionModule):**
- Primary code: `firmware/Integrall/src/modules/MotionModule.h`
- Configuration flag: `firmware/Integrall/src/config/IntegrallConfig.h` → add `#define INTEGRALL_ENABLE_MOTION`
- Expose in: `firmware/Integrall/src/Integrall.h` → add `enableMotion()` method
- Example: `firmware/Integrall/examples/MotionDetection.ino` → demonstrate usage
- Pattern: Follow existing module structure (header-only, compile-time conditional, non-blocking)

**New React Dashboard Component (e.g., AlertsPanel):**
- Component file: `weather_station/src/components/AlertsPanel.tsx`
- Types: Update `weather_station/src/types.ts` if new data structures needed
- Import in: `weather_station/src/App.tsx` → add `<AlertsPanel />` to render tree
- Styling: Use Tailwind CSS classes (already configured in `tailwind.config.js`)
- State: Lift state to App.tsx if shared with other components

**New Backend API Route (e.g., GET /api/stats):**
- Route: `IOT-Smart-Waste-Sorting/serverside/app.py` → add `@app.route('/api/stats')` function
- Database: Update in-memory `db` dict or persist to file
- Response: Return JSON via `jsonify()`
- Error Handling: Wrap in try/except, return appropriate HTTP status codes
- Client Call: Add Fetch in React component to `http://backend-url/api/stats`

**New Website Section (e.g., Community):**
- Component: `website/IntegrallV1/src/components/layout/CommunitySection.tsx`
- Import in: `website/IntegrallV1/src/App.tsx`
- Route: Consider adding React Router route if needed (Router already set up)
- Assets: Place images in `website/IntegrallV1/src/assets/`

**New Test File:**
- Arduino Test: `*.ino` in project root or `firmware/Integrall/examples/` (e.g., `TestMotionSensor.ino`)
- Python Test: `IOT-Smart-Waste-Sorting/serverside/test_*.py` (e.g., `test_stats.py`)
- React Test: `*.test.tsx` co-located with component or `__tests__/` directory (if Jest configured)

## Special Directories

**`firmware/Integrall/examples/`**
- Purpose: Ready-to-flash Arduino sketches
- Generated: No (manually created)
- Committed: Yes (version control)
- Usage: Copy to Arduino IDE, customize, flash to device

**`weather_station/node_modules/`**
- Purpose: npm dependencies
- Generated: Yes (from `package.json`)
- Committed: No (in `.gitignore`)
- Regenerate: `npm install` in `weather_station/` directory

**`website/IntegrallV1/node_modules/`**
- Purpose: npm dependencies
- Generated: Yes (from `package.json`)
- Committed: No (in `.gitignore`)
- Regenerate: `npm install` in `website/IntegrallV1/` directory

**`IOT-Smart-Waste-Sorting/serverside/uploads/`**
- Purpose: User-uploaded images for waste classification
- Generated: Yes (runtime file creation)
- Committed: No (in `.gitignore`)
- Cleanup: Safe to delete; files regenerate on upload

**`IOT-Smart-Waste-Sorting/serverside/yolov8*.pt`**
- Purpose: Pre-trained YOLOv8 object detection models
- Generated: No (downloaded or provided separately)
- Committed: No (too large; in `.gitignore`)
- Note: Optional; app can run without them if using Groq Vision instead

**`.planning/codebase/`**
- Purpose: Generated analysis documents
- Generated: Yes (by `/gsd-map-codebase`)
- Committed: Yes (version control for reference)
- Refresh: Run `/gsd-map-codebase --focus arch` to update

**`.git/`**
- Purpose: Git repository metadata
- Generated: Yes (by `git init`)
- Committed: No (special directory)
- Note: Repository is tracked; contains all commit history

---

*Structure analysis: 2026-09-09*
