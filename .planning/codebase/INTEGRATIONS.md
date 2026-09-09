# External Integrations

**Analysis Date:** 2026-09-09

## APIs & External Services

**AI Vision & Classification:**
- Groq LLM API - Computer vision inference for waste classification
  - SDK/Client: `groq` Python package
  - Model: `meta-llama/llama-4-scout-17b-16e-instruct` (vision-capable LLM)
  - Auth: Environment variable `GROQ_API_KEY`
  - Location: `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 9, 24, 60-99)
  - Purpose: Zero-shot waste material classification (plastic, metal, organic, paper, nylon, empty)
  - Image format: Base64-encoded JPEG via data URI
  - Response: JSON with `category`, `confidence` (0-1), and `object` description

**Machine Learning:**
- Ultralytics YOLO - Object detection framework
  - Package: `ultralytics`
  - Location: `IOT-Smart-Waste-Sorting/serverside/requirements.txt`
  - Purpose: Real-time object detection (alternative/supplementary to Groq vision)

**Image Processing:**
- OpenCV - Computer vision library
  - Package: `opencv-python`
  - Location: `IOT-Smart-Waste-Sorting/serverside/`
  - Purpose: Image preprocessing and manipulation

## Data Storage

**Databases:**
- Supabase PostgreSQL (BaaS)
  - Client: `@supabase/supabase-js` 2.57.4
  - Location: `weather_station/package.json`, `IOT-Smart-Waste-Sorting/client/project/package.json`
  - Connection: Environment variable (in `.env` files, contents not exposed)
  - Purpose: Real-time data synchronization, user authentication, sensor history storage
  - Status: Referenced in dependencies but usage commented as "don't have Supabase Realtime anymore" in `IOT-Smart-Waste-Sorting/client/project/src/hooks/useDetections.ts` (line 36)

**File Storage:**
- Local filesystem - Temporary image storage
  - Location: `uploads/` directory created at runtime in Flask backend
  - Implementation: `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 18-20, 122-135)
  - Purpose: Store uploaded waste detection images with UUID-based filenames
  - Access: HTTP endpoint `/uploads/<filename>` for retrieval (line 211-213)

**In-Memory Storage:**
- Python dictionary-based cache (development/testing)
  - Location: `weather_backend.py` (line 11-12), `IOT-Smart-Waste-Sorting/serverside/app.py` (line 30-45)
  - Purpose: Temporary telemetry and detection history (max 50-100 items)
  - Note: Production should use persistent database

**Caching:**
- None detected - Applications use polling for real-time updates

## Authentication & Identity

**Auth Provider:**
- Supabase Auth (configuration present but integration minimal)
  - Implementation: `@supabase/supabase-js` package installed but not actively used in visible code
  - Status: Planned infrastructure, not currently implemented in React components

**Custom/Local:**
- API Key-based authentication for ESP32 devices
  - Location: `firmware/Integrall/src/Integrall.h` (lines 20-22)
  - Field: `config.api_key` in DeviceConfig
  - Purpose: Device identification and telemetry endpoint authentication

## Monitoring & Observability

**Error Tracking:**
- None detected - No Sentry, Rollbar, or equivalent

**Logs:**
- Console logging in Python backends
  - Flask: `print()` statements for debug output
  - Location: `weather_backend.py` (line 34), `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 98, 109, 115-119)
- Browser console for frontend errors
- ESP32 serial monitor (via `monitor_speed = 115200` in `firmware/platformio.ini`)

**Status Endpoints:**
- Health check: `IOT-Smart-Waste-Sorting/serverside/app.py` (line 215-222)
  - Endpoint: `GET /api/health`
  - Response: JSON with status, engine, API connection, timestamp

## CI/CD & Deployment

**Hosting:**
- Not configured - Project appears to be local development only
- Potential deployment targets:
  - Web apps: Any Node.js-compatible hosting (Vercel, Netlify, AWS, GCP, etc.)
  - Python backends: Heroku, Railway, AWS Lambda, Google Cloud Run, etc.
  - ESP32: Over-the-air (OTA) updates via partition scheme (`firmware/platformio.ini`, line 32)

**CI Pipeline:**
- None detected - No GitHub Actions, GitLab CI, or Jenkins configuration
- No Docker/Dockerfile for containerization

**Version Control:**
- Git repository present (`.git/` directory)
- Recent commits related to dependencies and feature branches
- No automated deployment pipeline

## Environment Configuration

**Required env vars:**

### Weather Station (`weather_station/`)
- `VITE_API_URL` - Backend API endpoint (used in polling from localhost:5000)

### IoT Smart Waste Sorting (`IOT-Smart-Waste-Sorting/`)
**Client:**
- `VITE_API_URL` - Flask backend URL (default likely localhost:3000 for dev)

**Serverside:**
- `GROQ_API_KEY` - Groq API authentication token (critical for vision inference)

### Firmware (`firmware/Integrall/`)
- WiFi credentials: `config.wifi_ssid`, `config.wifi_password`
- Backend URL: `config.backend_url`
- API key: `config.api_key`
- Device ID: `config.device_id`

**Secrets location:**
- `.env` files (listed but contents not exposed per security policy)
- Environment variables set at runtime or in hosting platform secrets manager
- For ESP32: Hardcoded in sketch via DeviceConfig struct

## Webhooks & Callbacks

**Incoming:**
- `POST /telemetry` - ESP32 sends sensor telemetry
  - Location: `weather_backend.py` (lines 14-36)
  - Payload: JSON sensor data with temperature, humidity, pressure, etc.
  - Response: `{"status": "success", "received": data}`

- `POST /api/detect` - ESP32 uploads waste detection image for classification
  - Location: `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 113-179)
  - Payload: Multipart form with `image` file and `device_id`, or raw JPEG binary
  - Response: `{"category": string, "confidence": float, "object": string, "id": uuid}`

- `POST /api/log` - ESP32 sends remote log messages
  - Location: `IOT-Smart-Waste-Sorting/serverside/app.py` (lines 104-111)
  - Payload: `{"message": string}`
  - Response: `{"status": "ok"}`

**Outgoing:**
- No external webhook callbacks detected
- Internal polling mechanism:
  - Frontend polls `/api/live` every 4000ms for weather updates (`weather_station/src/App.tsx`, line 8)
  - Frontend polls `/api/detections` every 5000ms for waste detection updates (`IOT-Smart-Waste-Sorting/client/project/src/hooks/useDetections.ts`, line 37)

## API Endpoints Summary

**Weather Station Backend** (`weather_backend.py` runs on port 5000):
- `GET /api/live` - Get current sensor reading and history
- `POST /telemetry` - Receive telemetry from ESP32

**Waste Sorting Backend** (`IOT-Smart-Waste-Sorting/serverside/app.py` runs on port 3000):
- `POST /api/detect` - Analyze waste image with Groq vision
- `GET /api/detections` - Retrieve detection history (optional filters: device, category)
- `GET /api/devices` - List connected smart bins
- `GET /api/alerts` - Get unresolved alerts
- `POST /api/alerts/<id>/resolve` - Mark alert as resolved
- `POST /api/log` - Receive remote device logs
- `GET /api/health` - Health check with API connectivity status
- `GET /uploads/<filename>` - Retrieve uploaded detection images

---

*Integration audit: 2026-09-09*
