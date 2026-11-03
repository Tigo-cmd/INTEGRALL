# INTEGRALL: The Universal IoT Framework

**Build professional-grade IoT systems on ESP32 & Arduino with zero boilerplate.**

Integrall is a modular, event-driven firmware framework that simplifies hardware development. From simple sensor nodes to complex industrial safety systems, Integrall provides the scaffolding so you can focus on logic, not registers.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)](https://platformio.org/)

---

## The Vision
Integrall turns the fragmented world of Arduino libraries into a unified "Operating System" for your hardware.
- **Complexity Abstraction**: Control 17+ hardware modules with consistent, one-liner APIs.
- **Safety First**: Industrial-grade safety timeouts, interlock groups, and filtered sensor readings.
- **Scalable Core**: Compile-time module selection (pay only for what you use).
- **Zero Overhead**: Disabled modules compile to nothing — not a single byte of flash.

---

## Architecture & Project Structure

```text
INTEGRALL/
├── firmware/              # Modular C++ Library
│   └── Integrall/          # The core framework
│       └── src/
│           ├── Integrall.h       # Main header (only file users include)
│           ├── config/           # Compile-time module flags
│           ├── core/             # DeviceManager, Logger
│           └── modules/          # 17 hardware modules
├── website/               # React-based documentation & landing page
└── examples/              # Ready-to-flash project templates
```

### Firmware Compilation Flow
The firmware follows a **modular, compile-time-conditional architecture**. Users enable only target modules via `#define` flags, ensuring zero memory overhead for unused features.

```cpp
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>
Integrall::System integrall;
```

---

## Hardware Ecosystem (17+ Modules)
Integrall abstracts the complexity of these components into non-blocking, event-driven modules:

| | | | |
| :--- | :--- | :--- | :--- |
| **Relays** | **Power (INA219)** | **Rich Audio (MP3)** | **ESP32-CAM** |
| **Sensors & IMU** | **Time & GPS** | **Security Presets** | **Input UI** |
| **LCD / OLED** | **Wireless (LoRa/BLE)** | **SD Storage** | **Servo/Stepper** |
| **Buzzer** | **RGB LED** | **Keypad** | **Cellular (GSM)** |

### Module Highlights
1. **Relay Logic**: Auto-off safety timeouts, interlock groups, and hardware button overrides.
2. **Display Hub**: Smart row-cached scrolling for LCD and opaque hardware-accelerated progress bars for OLED.
3. **Sensor & IMU Engine**: Multi-sample averaging for HC-SR04, cached DHT/DS18B20/BME280 drivers, scientific psychrometrics (Magnus-Tetens dew point & NOAA Rothfusz heat index), and 6-axis MPU6050 fall/posture detection.
4. **Security Auth**: Integrated matrix keypad support with password buffer and non-blocking lockout management.
5. **Motion Control**: Non-blocking servo easing/sweeping (12-bit ESP32 ADC aware) and stepper rotation.
6. **Edge Vision & Cellular**: RAM-safe MJPEG streaming server & multipart uploader for ESP32-CAM, plus SIM800L GSM/SMS alerts.

---

## Quick Start (60 Seconds)

### Minimal Firmware Sketch
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

---

## Ready-Made Project Systems

Integrall ships with 5 complete project-level APIs that combine multiple modules into turnkey systems:

| Project | Modules Required | Lines of Code |
| :--- | :--- | :--- |
| **PIN Lock System** | Keypad + Relay (+ LCD optional) | 12 lines |
| **Alarm System** | Sensors + Relay (+ Buzzer optional) | 10 lines |
| **Parking Sensor** | Sensors (+ LCD, Buzzer optional) | 8 lines |
| **Weather Station** | Sensors/DHT (+ LCD optional) | 8 lines |
| **Smart Switch** | Relay + Sensors | 8 lines |

Each project needs just one call in `setup()` and one in `loop()`.

---

## Connectivity

The firmware includes full WiFi connectivity support for ESP32 and ESP8266 boards:

```cpp
#define INTEGRALL_ENABLE_WIFI
#include <Integrall.h>

void setup() {
    integrall.begin("MyWiFi", "Password123");
}

void loop() {
    integrall.handle();
}
```

Utility methods include: `isWiFiConnected()`, `getIPAddress()`, `getWiFiStrength()`, `httpGet()`, `httpPost()`, and more.

> **NOTE on Backend**: v1.0.0 includes a production-ready **Flask/Python Backend** for device telemetry and live visualization. The React dashboard is fully integrated with the firmware's automated telemetry sync.

---

## Data Flow (Current)

```text
        ┌────────────┐  ┌────────────┐  ┌────────────┐
        │  ESP32 #1  │  │  ESP32 #2  │  │  ESP32 #3  │
        └──────┬─────┘  └──────┬─────┘  └──────┬─────┘
               │               │               │
               └───── WiFi Network ────────────┘
                          │
                    Local HTTP APIs
                   (httpGet / httpPost)
```

*When the cloud backend is released, devices will automatically register and sync telemetry through a central FastAPI server.*

---

## Documentation
- **[Local User Guide (GUIDE.md)](GUIDE.md)**: Full offline reference for every module, API, and project system.
- **[Install Guide (DEPENDENCIES.md)](DEPENDENCIES.md)**: Required libraries for each hardware module.

## Roadmap
- [x] **v0.5 Core Modules**: LCD, OLED, Sensors, Relays, Buzzer, Blinker.
- [x] **v0.8 Expansion**: Camera, Rich Audio, Input UI, Storage, Time/GPS, Power, Stepper, Communication (BLE/LoRa).
- [x] **Project Presets**: Smart Lock, Alarm System, Weather Station, Parking Sensor, Smart Switch.
- [x] **v1.0 Backend**: 🐍 Flask cloud backend with device telemetry, live React dashboard, and redundant sensor analysis.
- [x] **v1.1.0 Firmware & Core**: 100% non-blocking state machines, multi-arch (ESP32/ESP8266/AVR) & ArduinoJson v7 support, cached sensor/LCD drivers, MPU6050 IMU fall detection, and SIM800L Cellular SMS/Voice.
- [ ] **v1.2 Fleet**: OTA Firmware Updates and MQTT Broker Integration.

## Contributing
Integrall is an open-source project. We welcome contributions for new hardware modules, bug fixes, and documentation improvements.

---
**Built with care for the IoT Community.**
*Simplify the physical. Scale the digital.*
