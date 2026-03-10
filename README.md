# 🌐 INTEGRALL: The Universal IoT Framework

**Build professional-grade IoT systems on ESP32 & Arduino with zero boilerplate.**

Integrall is a modular, event-driven, full-stack framework that simplifies hardware development. From simple sensor nodes to complex industrial safety systems, Integrall provides the scaffolding so you can focus on logic, not registers.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)
[![FastAPI](https://img.shields.io/badge/FastAPI-Production--Ready-00a393.svg)](https://fastapi.tiangolo.com/)

---

## 🎯 The Vision
Integrall turns the fragmented world of Arduino libraries into a unified "Operating System" for your hardware.
- **Complexity Abstraction**: Control 17+ hardware modules with consistent, one-liner APIs.
- **Safety First**: Industrial-grade safety timeouts, interlock groups, and filtered sensor readings.
- **Scalable Core**: Compile-time module selection (Pay-only-for-what-you-use).
- **Backend-Native**: Automatic device registration, telemetry, and command polling.

---

## 🏗️ Architecture & Project Structure

Integrall is a **full-stack** ecosystem consisting of a C++ Firmware library, a Python FastAPI backend, and a React-based developer hub.

```text
INTEGRALL/
├── firmware/              # Modular C++ Library
│   └── Integrall/          # The core framework (src/modules, src/core)
├── backend/               # Professional FastAPI/Python server
├── website/               # React-based documentation & landing page
└── examples/              # 15+ ready-to-flash project templates
```

### Firmware Compilation Flow
The firmware follows a **modular, compile-time-conditional architecture**. Users enable only target modules via `#define` flags, ensuring zero memory overhead for unused features.

```cpp
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>
Integrall::System integrall;
```

---

## 🔌 Hardware Ecosystem (17+ Modules)
Integrall abstracts the complexity of these components into non-blocking, event-driven modules:

| | | | |
| :--- | :--- | :--- | :--- |
| 🎚️ **Relays** | ⚡ **Power (INA219)** | 🔊 **Rich Audio (MP3)** | 📷 **ESP32-CAM** |
| 🌡️ **Sensors (10+)** | ⏱️ **Time & GPS** | 🛡️ **Security Presets** | 🕹️ **Input UI** |
| 📺 **LCD / OLED** | 📡 **Wireless (LoRa/BLE)** | 💾 **SD Storage** | ⚙️ **Stepper/Servo** |

### Detailed Module Highlights
1.  **Relay Logic**: Auto-off safety timeouts, interlock groups, and hardware button overrides.
2.  **Display Hub**: Smart scrolling for LCD and hardware-accelerated progress bars for OLED.
3.  **Sensor Engine**: Multi-sample averaging for HC-SR04, semantic motion sensing, and high-precision environmental monitoring (BME280).
4.  **Security Auth**: Integrated matrix keypad support with password buffer and lockout management.
5.  **Motion Control**: Non-blocking servo easing/sweeping and stepper rotation.
6.  **Edge Vision**: MJPEG streaming server for ESP32-CAM with zero-boilerplate setup.

---

## 🚀 Quick Start (60 Seconds)

### 1. Minimal Firmware Sketch
```cpp
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_LCD
#include <Integrall.h>

Integrall::System integrall;

void setup() {
    integrall.begin();
    int pump = integrall.enableRelay(5); 
    integrall.lcdPrint("Pump Ready", 0, 0);
}

void loop() {
    integrall.handle(); // Processes background logic/safety
}
```

### 2. Backend & Communication
The Python backend (FastAPI) manages device registration, remote command dispatch, and telemetry. All communication is secured via `X-API-Key` authentication.

```bash
cd backend
./start_backend.sh  # Starts at http://localhost:8000
```

---

## 🌐 Connectivity & Data Flow

```text
                    ┌──────────────────────┐
                    │  Developer Hub       │
                    │  (React / Website)   │
                    └──────────┬───────────┘
                               │ Reference
                               ▼
                    ┌──────────────────────┐
                    │   FastAPI Backend    │
                    │   (Python / Database)│
                    └──────────┬───────────┘
                               │ HTTP REST API
               ┌───────────────┼───────────────┐
               ▼               ▼               ▼
        ┌────────────┐  ┌────────────┐  ┌────────────┐
        │  ESP32 #1  │  │  ESP32 #2  │  │  ESP32 #3  │
        └────────────┘  └────────────┘  └────────────┘
```

---

## 📖 Documentation
Detailed guides are available in two formats:
- **[Website & Live Docs](https://integrall-docs.vercel.app)**: Interactive tutorials, project showcases, and API search.
- **[Local User Guide (GUIDE.md)](GUIDE.md)**: Full offline reference for every module and safety feature.
- **[Install Guide (DEPENDENCIES.md)](DEPENDENCIES.md)**: Required libraries for various hardware modules.

## 📉 Roadmap
- [x] **v0.5 Core Modules**: LCD, OLED, Sensors, Relays, Buzzer, Blinker.
- [x] **v0.8 Expansion**: Camera, Rich Audio, Input UI, Storage, Time/GPS, Power, Stepper.
- [x] **Project Presets**: Smart Lock, Alarm System, Weather Station, Parking Sensor.
- [ ] **v1.0 Final**: Fleet Management, OTA Updates, MQTT Broker Integration.

## 🤝 Contributing
Integrall is an open-source project. We welcome contributions for new hardware modules, bug fixes, and documentation improvements. See [CONTRIBUTING.md](CONTRIBUTING.md).

---
**Built with ❤️ for the IoT Community.**
*Simplify the physical. Scale the digital.*
