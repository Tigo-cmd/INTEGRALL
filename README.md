# Integrall IoT Framework v0.1

**Unified IoT firmware framework for ESP32 - simplifies hardware integration and backend communication.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![FastAPI](https://img.shields.io/badge/FastAPI-0.104+-00a393.svg)](https://fastapi.tiangolo.com/)

## 🎯 Vision

Integrall abstracts complex IoT development into a simple, unified experience:

- **For Hobbyists**: Connect ESP32 to the cloud in 10 minutes, < 20 lines of code
- **For Students**: Learn IoT without wrestling with low-level libraries
- **For Startups**: Prototype fast with production-ready architecture
- **For Developers**: Modular, scalable, event-driven architecture

## ✨ Features

### Firmware (ESP32/Arduino)
- ✅ **Zero-config WiFi** - Auto-connect with persistent credentials
- ✅ **Plug-and-play modules** - Relay, Keypad, Display (OLED/LCD)
- ✅ **Safety-first design** - Auto-off timeouts, interlock groups, debounced inputs
- ✅ **Event-driven architecture** - Non-blocking, responsive system
- ✅ **Memory optimized** - Compile only what you need, lazy initialization
- ✅ **Backend integration** - Automatic registration, command polling, telemetry

### Backend (Python/FastAPI)
- ✅ **Async architecture** - Handles thousands of concurrent devices
- ✅ **Auto-generated API docs** - Swagger UI at `/docs`
- ✅ **Device command queue** - Reliable command dispatch with status tracking
- ✅ **SQLite/PostgreSQL** - Start simple, scale to production
- ✅ **API key security** - Simple but effective device authentication
- ✅ **Real-time dashboard** - Device status, command history, telemetry

## 🚀 Quick Start

### 1. Hardware Setup (5 minutes)

**Required:**
- ESP32 development board
- Relay module (or LED for testing)
- USB cable

**Wiring:**
```
ESP32 GPIO 5 → Relay IN (Signal)
ESP32 3.3V   → Relay VCC (if required)
ESP32 GND    → Relay GND
```

### 2. Backend Setup (3 minutes)

```bash
cd backend
./start_backend.sh  # or start_backend.bat on Windows
```

The backend will start at `http://localhost:8000`
- API Docs: http://localhost:8000/docs
- Health Check: http://localhost:8000/health

### 3. Firmware Setup (2 minutes)

**Option A: PlatformIO (Recommended)**
```bash
cd firmware
pio run --target upload
pio device monitor
```

**Option B: Arduino IDE**
1. Copy `firmware/Integrall` to your Arduino libraries folder
2. Open `examples/BasicRelay/BasicRelay.ino`
3. Update WiFi credentials and backend URL
4. Upload to ESP32

### 4. See It Work!

1. Open Serial Monitor (115200 baud) - see device register with backend
2. Open http://localhost:8000/docs
3. POST to `/api/commands` to turn relay on/off:
```json
{
  "device_id": "INT_A1B2C3D4E5F6",
  "command_type": "relay_on",
  "parameters": {"pin": 5}
}
```

## 📁 Project Structure

```
integrall/
├── firmware/              # ESP32 Arduino library
│   ├── Integrall/
│   │   ├── src/
│   │   │   ├── Integrall.h          # Main user API
│   │   │   ├── config/
│   │   │   │   └── IntegrallConfig.h # Compile-time configuration
│   │   │   ├── core/
│   │   │   │   ├── DeviceManager.h   # WiFi, backend, identity
│   │   │   │   └── Logger.h          # Debug logging
│   │   │   └── modules/
│   │   │       ├── RelayModule.h     # Relay control with safety
│   │   │       └── (Keypad, OLED coming in v0.2)
│   │   └── examples/
│   │       ├── BasicRelay/           # Simple relay control
│   │       └── FullDemo/             # All features
│   └── platformio.ini       # PlatformIO configuration
│
├── backend/                 # Python FastAPI backend
│   ├── app/
│   │   ├── main.py          # FastAPI app entry point
│   │   ├── core/
│   │   │   ├── config.py    # Settings management
│   │   │   └── database.py  # Async SQLAlchemy setup
│   │   ├── models/
│   │   │   └── device.py    # Database models
│   │   ├── schemas/
│   │   │   └── device.py    # Pydantic schemas
│   │   ├── services/
│   │   │   └── crud.py      # Database operations
│   │   └── api/
│   │       └── routes/
│   │           ├── devices.py   # Device registration
│   │           ├── commands.py # Command dispatch
│   │           └── telemetry.py # Data ingestion
│   ├── requirements.txt
│   └── start_backend.sh
│
└── docs/                    # Documentation
```

## 💻 Usage Examples

### Basic Relay Control
```cpp
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>

Integrall::System integrall;

void setup() {
    Integrall::DeviceConfig config;
    config.wifi_ssid = "MyWiFi";
    config.wifi_password = "secret";
    config.backend_url = "http://192.168.1.100:8000";
    config.api_key = "my-api-key";
    
    integrall.begin(config);
    integrall.enableRelay(5);  // GPIO 5
}

void loop() {
    integrall.handle();
}
```

### With Safety Features
```cpp
int relay = integrall.enableRelay(5, true, "Heater");
integrall.relaySetTimeout(relay, 30000);     // Auto-off after 30s
integrall.relaySetInterlock(relay, 1);     // Group 1 interlock
integrall.relayAttachButton(relay, 4);      // Physical button on GPIO 4
```

### Send Telemetry
```cpp
StaticJsonDocument<256> doc;
doc["temperature"] = 24.5;
doc["humidity"] = 60;
integrall.sendTelemetry(doc);
```

## 🔧 Configuration

### Firmware (Compile-time)
```cpp
// In your sketch, BEFORE including Integrall.h:
#define INTEGRALL_ENABLE_RELAY      // Enable relay module
#define INTEGRALL_ENABLE_KEYPAD     // Enable keypad module (v0.2)
#define INTEGRALL_ENABLE_OLED       // Enable OLED display (v0.2)
#define INTEGRALL_DEBUG_LEVEL 2     // 0=none, 1=errors, 2=warn, 3=info, 4=verbose

#include <Integrall.h>
```

### Backend (Environment)
```bash
# .env file
DATABASE_URL=sqlite+aiosqlite:///./integrall.db
API_KEY=your-secret-api-key
SECRET_KEY=your-jwt-secret
DEBUG=true
```

## 📊 API Reference

### Device Registration
```http
POST /api/devices/register
X-API-Key: your-api-key

{
  "device_id": "INT_A1B2C3D4E5F6",
  "ip_address": "192.168.1.105",
  "mac_address": "A1:B2:C3:D4:E5:F6",
  "firmware_version": "0.1.0"
}
```

### Send Command
```http
POST /api/commands
X-API-Key: your-api-key

{
  "device_id": "INT_A1B2C3D4E5F6",
  "command_type": "relay_on",
  "parameters": {"pin": 5, "safety_timeout_ms": 30000}
}
```

### Poll Commands (Device)
```http
GET /api/devices/{device_id}/commands
X-API-Key: your-api-key
```

### Submit Telemetry
```http
POST /api/telemetry
X-API-Key: your-api-key

{
  "device_id": "INT_A1B2C3D4E5F6",
  "data": {
    "temperature": 24.5,
    "relay_states": [true, false]
  }
}
```

## 🛡️ Safety Features

Integrall includes multiple safety mechanisms:

1. **Safety Timeouts** - Relays automatically turn off after specified duration
2. **Interlock Groups** - Prevent simultaneous activation of conflicting relays
3. **Debounced Inputs** - Hardware buttons use 50ms software debouncing
4. **Memory Protection** - Compile-time module selection prevents bloat
5. **API Key Auth** - All device endpoints require authentication
6. **State Validation** - Commands validate device existence before queueing

## 📈 Roadmap

### v0.1 (Current)
- ✅ ESP32 WiFi + HTTP backend
- ✅ Relay module with safety features
- ✅ FastAPI backend with SQLite
- ✅ Device registration and command polling

### v0.2 (Next)
- 🔄 Keypad module (3x4, 4x4 matrix)
- 🔄 OLED/LCD display modules
- 🔄 MQTT support (optional)
- 🔄 WebSocket real-time push
- 🔄 OTA firmware updates

### v0.3
- 🔄 ESP8266 support
- 🔄 Additional sensors (DHT, BME280)
- 🔄 Voice integration hooks
- 🔄 Modular library packages

### v1.0
- 🔄 Industrial features (fleet management)
- 🔄 Role-based access control
- 🔄 Cloud SaaS deployment
- 🔄 PostgreSQL/MongoDB support

## 🤝 Contributing

Contributions welcome! Please read our [Contributing Guide](CONTRIBUTING.md).

Areas where help is needed:
- Additional hardware modules (sensors, actuators)
- Frontend dashboard (React/Vue)
- Documentation and tutorials
- Testing and bug reports

## 📄 License

MIT License - see [LICENSE](LICENSE) file.

## 🙏 Acknowledgments

- FastAPI team for the excellent async web framework
- PlatformIO team for professional embedded tooling
- Arduino community for open-source hardware ecosystem

---

**Built with ❤️ by Emmanuel TIGO and contributors**

*Simplify IoT. Focus on what matters.*
