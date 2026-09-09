# Technology Stack

**Analysis Date:** 2026-09-09

## Languages

**Primary:**
- TypeScript 5.5-5.9 - Frontend applications and build tooling across `weather_station/`, `website/IntegrallV1/`, `IOT-Smart-Waste-Sorting/client/`
- Python 3.x - Backend services: `weather_backend.py`, `IOT-Smart-Waste-Sorting/serverside/app.py`
- C++ / Arduino - Embedded firmware: `firmware/Integrall/`, test files (`.ino`, `.cpp`)
- JSON - Configuration and data serialization

**Secondary:**
- Bash - Setup and test scripts at root level
- Shell - Platform configuration via PlatformIO

## Runtime

**Environment:**
- Node.js 16+ (implied by Vite and package.json ES modules)
- Python 3.7+ (Flask framework compatibility)
- ESP32 / ESP8266 microcontroller (Arduino runtime via PlatformIO)
- Arduino IDE / PlatformIO (firmware build environment)

**Package Manager:**
- npm (JavaScript/TypeScript projects)
  - Lockfile: `package-lock.json` present in `weather_station/`, `website/IntegrallV1/`, `IOT-Smart-Waste-Sorting/client/project/`
- pip (Python dependency management)
  - Requirements file: `IOT-Smart-Waste-Sorting/serverside/requirements.txt`

## Frameworks

**Core:**
- React 18.3.1 - UI framework for `weather_station/` and `IOT-Smart-Waste-Sorting/client/project/`
- React 19.2.0 - Newer version in `website/IntegrallV1/`
- Vite 5.4.2 - Build tool and dev server (primary, used in weather_station and IoT client)
- Vite 7.3.1 - Newer version in `website/IntegrallV1/`
- Flask - Python web framework for backend services (`weather_backend.py`, `IOT-Smart-Waste-Sorting/serverside/app.py`)
- Arduino Framework - Embedded systems development via PlatformIO (`firmware/platformio.ini`)

**UI & Styling:**
- Tailwind CSS 3.4.x - Utility-first CSS framework in all web projects
- Radix UI / React Slot - Component library in `website/IntegrallV1/`
- Framer Motion 12.35.2 - Animation library in `website/IntegrallV1/`
- Lucide React 0.344.0 - Icon library in `weather_station/` and IoT client

**Routing & State:**
- React Router DOM 7.13.1 - Client-side routing in `website/IntegrallV1/`
- Local state management via `useState` and `useCallback` hooks (no Redux/Zustand detected)

**Testing & Dev:**
- ESLint 9.9.1 - JavaScript linting
- TypeScript 5.5.3-5.9.3 - Type checking
- Autoprefixer 10.4.18 - CSS vendor prefixing
- PostCSS 8.4.35 - CSS processing pipeline

**Data Visualization:**
- Recharts 3.8.0 - React chart library for sensor data visualization in `weather_station/`

## Key Dependencies

**Critical:**
- `@supabase/supabase-js` 2.57.4 - Backend-as-a-Service for database and auth (listed in `weather_station/package.json`, `IOT-Smart-Waste-Sorting/client/project/package.json`)
- `groq` - Python SDK for Groq AI vision model (in `IOT-Smart-Waste-Sorting/serverside/requirements.txt`)
- `ArduinoJson` 6.21.3 - JSON serialization for ESP32 firmware (`firmware/platformio.ini`)

**Infrastructure:**
- `flask` - Lightweight Python web framework
- `flask-cors` - CORS support for cross-origin requests
- `ultralytics` - YOLO object detection library (in `IOT-Smart-Waste-Sorting/serverside/`)
- `opencv-python` - Computer vision library for image processing
- `numpy` - Numerical computing library
- `Pillow` - Python Image Library for image handling
- `python-dotenv` - Environment variable management
- `react-dom` 18.3.1, 19.2.0 - React DOM rendering
- `react-is` 19.2.4 - React type checking utilities
- `react-use-measure` 2.1.7 - React hook for measuring DOM elements

**Build & Development:**
- `@vitejs/plugin-react` 4.3.1, 5.1.1 - Vite React integration
- `@eslint/js` 9.9.1, 9.39.1 - ESLint core
- `@types/react` 18.3.5, 19.2.7 - TypeScript React types
- `@types/react-dom` 18.3.0, 19.2.3 - TypeScript React DOM types
- `typescript-eslint` 8.3.0, 8.48.0 - TypeScript ESLint integration
- `eslint-plugin-react-hooks` 5.1.0, 7.0.1 - ESLint React Hooks rules
- `eslint-plugin-react-refresh` 0.4.11, 0.4.24 - Vite React Fast Refresh support

**Styling & Utilities:**
- `class-variance-authority` 0.7.1 - CSS class composition in `website/IntegrallV1/`
- `clsx` 2.1.1 - Utility for conditional classNames
- `tailwind-merge` 3.5.0 - Intelligent Tailwind class merging

## Configuration

**Environment:**
- Environment files detected:
  - `.env` in `weather_station/` - Contains Supabase/backend config
  - `.env` in `IOT-Smart-Waste-Sorting/serverside/` - Contains Groq API key and Flask config
- Variables sourced via:
  - `import.meta.env.VITE_*` for frontend (Vite environment variables)
  - `process.env` or `os.getenv()` for Python backends

**Build:**
- `tsconfig.json` in each TypeScript project - TypeScript compiler configuration
- `tsconfig.app.json` referenced for app-specific type checking
- `vite.config.ts` in Vite projects - Build and dev server configuration
  - Path alias: `@` resolves to `src/` directory in `website/IntegrallV1/`
- `platformio.ini` in firmware - PlatformIO build configuration for ESP32
  - Board: `esp32dev`
  - Framework: Arduino
  - Monitor speed: 115200
  - Upload speed: 921600
  - Build flags: Debug levels configurable via `INTEGRALL_DEBUG_LEVEL`
  - File system: littlefs for web interface support
  - Partition scheme: default.csv for OTA updates

**Development:**
- `.gitignore` present - Standard Git exclude patterns
- `DEPENDENCIES.md` - Comprehensive firmware library requirements documentation

## Platform Requirements

**Development:**
- Node.js 16+ with npm
- Python 3.7+
- Arduino IDE or PlatformIO CLI
- ESP32 board support (Espressif Systems)
- TypeScript compiler (dev dependency)
- Modern code editor with TypeScript support

**Production:**
- Node.js runtime for React apps (via SSR) or static file server
- Python 3.7+ runtime for Flask backends
- ESP32 microcontroller with WiFi capability
- Backend database (Supabase or compatible PostgreSQL)
- Groq API access for computer vision inference
- File storage for uploaded images (`uploads/` directory in Flask backend)

---

*Stack analysis: 2026-09-09*
