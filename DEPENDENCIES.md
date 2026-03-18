# 🛠️ Integrall Framework: Dependencies

To use the full power of the **Integrall Framework**, you need to install a few external libraries. Most can be installed directly from the **Arduino Library Manager** (Ctrl+Shift+I or Cmd+Shift+I).

---

## 📦 Core Dependencies (Required)
These are essential for the framework to compile and connect to the backend.

| Library Name | Author | Purpose | Installation |
| :--- | :--- | :--- | :--- |
| **ArduinoJson** | Benoit Blanchon | Data handling & JSON | Search "ArduinoJson" |

---

## 🔌 Module Dependencies (Required for specific modules)
If you enable a module in your code (e.g., `#define INTEGRALL_ENABLE_LCD`), you **must** install the corresponding library.

| Module Flag | Library to Install | Note |
| :--- | :--- | :--- |
| `INTEGRALL_ENABLE_LCD` | **LiquidCrystal I2C** | By Frank de Brabander |
| `INTEGRALL_ENABLE_SERVO` | **ESP32Servo** | Required for ESP32 |
| `INTEGRALL_ENABLE_KEYPAD` | **Keypad** | By Mark Stanley |
| `INTEGRALL_ENABLE_OLED` | **Adafruit SSD1306** | By Adafruit |
| `INTEGRALL_ENABLE_OLED` | **Adafruit GFX Library** | Dependency for OLED |
| `INTEGRALL_ENABLE_SENSORS`| **DHT sensor library** | By Adafruit (for temp/hum) |
| `INTEGRALL_ENABLE_SENSORS`| **DallasTemperature** | For waterproof probes |
| `INTEGRALL_ENABLE_SENSORS`| **OneWire** | Required for DS18B20 |
| `INTEGRALL_ENABLE_SENSORS`| **Adafruit BME280** | For high-precision env |
| `INTEGRALL_ENABLE_SENSORS`| **Adafruit Unified Sensor** | Required for DHT/BME |
| `INTEGRALL_ENABLE_INPUT`  | **MFRC522** | Wireless RFID cards |
| `INTEGRALL_ENABLE_INPUT`  | **IRremote** | TV/Remote signal parsing |
| `INTEGRALL_ENABLE_TIME`   | **TinyGPS++** | Satellite/GPS logic |
| `INTEGRALL_ENABLE_TIME`   | **RTClib** | Hardware RTC (DS3231) |
| `INTEGRALL_ENABLE_COMM`   | **LoRa** | Long-range radio comms |
| `INTEGRALL_ENABLE_AUDIO`  | **DFRobotDFPlayerMini** | MP3 playback via Serial |
| `INTEGRALL_ENABLE_POWER`  | **Adafruit INA219** | Current/Volt monitoring |

---

## 🚀 Recommended for ESP32-CAM
If you are building the **Smart Camera** or using any AI-Thinker/Wrover modules:

1.  **ESP32 Board Support**: Ensure you have `esp32` by Espressif Systems installed in your **Boards Manager** (use version 2.0.x or 3.0.x).
2.  **No Extra Library Needed**: The camera driver (`esp_camera.h`) is built into the ESP32 board package, so you don't need to install a separate "Camera" library.

---

## 💡 Quick Installation Guide
1. Open **Arduino IDE**.
2. Go to **Sketch** -> **Include Library** -> **Manage Libraries**.
3. Search for the names in bold above.
4. Click **Install**.
