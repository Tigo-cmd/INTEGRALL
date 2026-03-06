# 📖 Integrall Framework: User Guide

Welcome to the **Integrall** documentation. This guide is updated in real-time as the framework evolves to help you build fast, safe, and powerful IoT projects.

---

## 🛠️ Getting Started (The "Integrall" Way)
The philosophy of Integrall is **Complexity Abstraction**. We hide the "ugly" setup so you can write logic in one line.

### 1. Minimal Sketch Structure (Offline Mode)
By default, Integrall stays **OFFLINE**. This is perfect for simple projects (Standard Arduino, Serial to LCD, etc.) and produces ZERO WiFi logs.

```cpp
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_LCD
#include <Integrall.h>

Integrall::System integrall;
```

### 2. Enabling WiFi
If you need connectivity, you **MUST** enable it explicitly BEFORE including the library.

```cpp
#define INTEGRALL_ENABLE_WIFI
#include <Integrall.h>
```

---

## 🌐 Connectivity & Cloud

Integrall handles the complexity of WiFi reconnections and Backend synchronization. You can provide credentials in two ways:

### 1. Global (The "Set and Forget" way)
Best for regular development. Edit `Integrall/src/config/IntegrallConfig.h`:
```cpp
#define INTEGRALL_DEFAULT_SSID     "YourWifiName"
#define INTEGRALL_DEFAULT_PASS     "YourWifiPassword"
#define INTEGRALL_DEFAULT_BACKEND  "http://YourIp:8000"
```
In your sketch, you just write:
```cpp
integrall.begin(); // pulls from global config automatically
```

### 2. Manual (The "Portable" way)
Best for sharing code or moving between networks.
```cpp
void setup() {
    Integrall::DeviceConfig config;
    config.wifi_ssid = "SecretNetwork";
    config.wifi_password = "Password123";
    config.backend_url = "http://api.integrall.io";
    config.api_key = "your-key";
    
    integrall.begin(config);
}
```

> [!IMPORTANT]
> **Manual Config** still requires `#define INTEGRALL_ENABLE_WIFI` at the top of your sketch to activate the networking hardware!

---

## 🔌 Hardware Modules

### 1. LCD Display (I2C)
Hides all the `Wire.h` and address management. 
- **Setup**: `#define INTEGRALL_ENABLE_LCD`
- **Commands**:
  - `integrall.lcdPrint("Text", col, row);` - Direct printing.
  - `integrall.lcdClear();` - Clear the screen.

### 2. Servo Motors
Works on **ESP32** and **Arduino Uno** with the same code.
- **Setup**: `#define INTEGRALL_ENABLE_SERVO`
- **One-Liners**:
  - `integrall.setServo(pin, angle);` - Move to angle (0-180).
  - `integrall.setServoFromAnalog(servo_pin, pot_pin);` - Automatically maps a potentiometer (Analog pin) to the motor angle.
  - `integrall.sweepServo(pin, speed_ms);` - **Non-blocking** sweep. The motor moves back and forth while your code keeps running (no `delay` used!).

### 3. Sensors (Ultrasonic, PIR, Analog)
No more complex math or timing logic.
- **Setup**: `#define INTEGRALL_ENABLE_SENSORS`
- **Smart Features**:
  - `float cm = integrall.readDistance(trig, echo, samples);` - Measures distance. The `samples` parameter (default 3) automatically filters out noise for a stable reading.
  - `bool near = integrall.isNear(trig, echo, threshold_cm);` - One-liner proximity alert. Returns `true` if an object is closer than the threshold.
  - `int light = integrall.readLightPercent(A0);` - Read LDR light intensity.
  - `int raw = integrall.readAnalogPercent(A0);` - Returns a clean 0-100% value from any analog sensor.
  - `bool motion = integrall.isTriggered(PIR_PIN);` - Simple true/false trigger for motion sensors.

### 4. Smart Relays
Standard relay control with professional safety features.
- **Setup**: `#define INTEGRALL_ENABLE_RELAY`
- **Key Features**:
  - `integrall.enableRelay(pin, active_low, "Name");` - Initialize a relay.
  - `integrall.relayOn(index);` / `integrall.relayOff(index);` / `integrall.relayToggle(index);` - Control.
  - `integrall.relaySetTimeout(index, 30000);` - **Auto-off** safety timer (30 seconds).
  - `integrall.relaySetInterlock(index, group);` - Prevent two relays in the same group turning on simultaneously (safety).
  - `integrall.relayAttachButton(index, pin);` - Attach a physical button to a relay (hardware override).

### 5. Matrix Keypad
Hides row/column scanning and debounce logic completely.
- **Setup**: `#define INTEGRALL_ENABLE_KEYPAD`
- **Commands**:
  - `integrall.enableKeypad(rowPins, colPins);` - Initialize. Works with 4x4 or 4x3 keypads.
  - `char k = integrall.keypadGetKey();` - Read the currently pressed key (raw).
  - `integrall.keypadCapture(maxLen);` - Build a string from keypresses. `*` = Backspace, `#` = Enter.
  - `integrall.keypadCheckPin("1234");` - Validate string against a PIN. Clears buffer automatically.

### 6. OLED Display (SSD1306)
Native I2C OLED support with text, values, and progress bars.
- **Setup**: `#define INTEGRALL_ENABLE_OLED`
- **Commands**:
  - `integrall.oledPrint("Text", col, row, clear);` - Print text at position.
  - `integrall.oledPrintValue("Temp: ", 24.5, 0);` - Print a labelled number value.
  - `integrall.oledBar(75);` - Draw a progress bar at 75%.
  - `integrall.oledClear();` - Clear the entire screen.

### 7. Buzzer Alerts
Supports single beeps, patterns, and ready-to-use alerts.
- **Setup**: `#define INTEGRALL_ENABLE_BUZZER`
- **Commands**:
  - `integrall.enableBuzzer(pin);` - Initialize.
  - `integrall.buzzerBeep(ms);` - Single beep.
  - `integrall.buzzerAlert();` - Triple rapid beep for danger/alarm.
  - `integrall.buzzerSuccess();` / `integrall.buzzerFail();` - Melodic feedback.

### 8. RGB LED
Control multi-color LEDs with named colors or RGB values.
- **Setup**: `#define INTEGRALL_ENABLE_RGB`
- **Commands**:
  - `integrall.enableRGB(rPin, gPin, bPin, commonAnode);` - Initialize.
  - `integrall.setColor("blue");` - Set by name (red, green, blue, yellow, cyan, white, purple, orange).
  - `integrall.setRGB(r, g, b);` - Custom color balance.
  - `integrall.rgbBlink(r, g, b, ms);` - **Non-blocking** color blink.

### 9. ESP32-CAM Video Stream
Professional MJPEG streaming with zero boilerplate.
- **Setup**: `#define INTEGRALL_ENABLE_CAMERA`
- **Pick your Board**: Simply define your model at the **very top** of your sketch (e.g., `#define CAMERA_MODEL_AI_THINKER`).
- **Supported Models**: AI-Thinker, Wrover Kit, ESP-EYE, M5Stack (all versions), TTGO T-Journal, Xiao ESP32S3, and more.
- **Commands**:
  - `integrall.enableCamera();` - Inits hardware and **auto-starts** a web server on Port 81 as soon as WiFi connects.
  - `const char* url = integrall.getCameraStreamURL();` - Get the link to your live video feed.
  - `integrall.startCameraServer();` - Manually start/restart the server if needed.

**Smart Memory Management**: Integrall automatically detects if your board has **PSRAM**. If it does, it enables High-Quality UXGA (1600x1200) streaming. If not, it safely downscales to prevent crashes.

> [!NOTE]
> **Backend Status**: Cloud backend features are currently **DISCONNECTED** by default in this version to focus on local library reliability. Telemetry and remote commands are disabled.

**Why use this?** A standard ESP32-CAM sketch is over 200 lines of complex pin mapping and HTTP handling. Integrall reduces this to **one line**.

### 10. Non-Blocking Blinker
The ultimate replacement for the "Blink" example. No `delay()` needed.
- **Commands**:
  - `integrall.blink(pin, interval);` - Start blinking in the background.
  - `integrall.stopBlink();` - Stop the blinking.

**Why use this?** Traditional `delay(1000)` freezes your ESP32. With `integrall.blink()`, your LED flashes while your WiFi and sensors keep working perfectly.

**Example – Sensor Dashboard:**
```cpp
void loop() {
    float dist = integrall.readDistance(5, 6);
    integrall.oledPrint("Distance:", 0, 0, true);
    integrall.oledPrintValue("cm: ", dist, 1);
    integrall.oledBar(integrall.getWiFiStrength() + 100); // Signal bar
}
```

---

## 🔐 High-Level Lock System
The most powerful feature of Integrall — a complete PIN-code lock system in **2 function calls**.

**Requires**: `INTEGRALL_ENABLE_KEYPAD` + `INTEGRALL_ENABLE_RELAY`
**Optional**: Add `INTEGRALL_ENABLE_LCD` for automatic feedback messages.

### What it handles automatically:
| Feature | Automatic |
| :--- | :---: |
| Asterisk display as user types | ✅ |
| Backspace (`*`) and Submit (`#`) | ✅ |
| "Unlocked" / "Access Denied" messages | ✅ |
| Remaining attempts counter on LCD | ✅ |
| Lockout after N wrong attempts | ✅ |
| Auto-lock relay after unlock duration | ✅ |

### Setup (one call in `setup()`):
```cpp
// lockSetup(pin, relayIndex, unlockDuration_ms, maxWrongAttempts)
integrall.lockSetup("1234", door, 5000, 3);
```

### Run (one call in `loop()`):
```cpp
integrall.lockUpdate(); // Handles EVERYTHING
```

### Complete Sketch (the whole project!):
```cpp
#define INTEGRALL_ENABLE_KEYPAD
#define INTEGRALL_ENABLE_LCD
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>

Integrall::System integrall;
byte rowPins[4] = {19, 18, 5, 17};
byte colPins[4] = {16,  4, 2, 15};

void setup() {
    integrall.begin();
    int door = integrall.enableRelay(13, true, "Door");
    integrall.enableKeypad(rowPins, colPins);
    integrall.lockSetup("1234", door, 5000, 3);
}

void loop() {
    integrall.lockUpdate();
    integrall.handle();
}
```


### 🚨 Alarm System
A professional security system with PIR motion detection, cooldown safety, and sound/light alerts.
- **Requires**: `INTEGRALL_ENABLE_SENSORS` + `INTEGRALL_ENABLE_RELAY`
```cpp
void setup() {
    integrall.begin();
    int siren = integrall.enableRelay(13, true, "Siren");
    // setupAlarm(pirPin, relayIndex, cooldownMs)
    integrall.alarmSetup(19, siren, 10000); 
}

void loop() {
    integrall.alarmUpdate(); // Automatically manages detection & triggers
    integrall.handle();
}
```

### 🚗 Parking Sensor
Visual and audible distance assistance for garages or robots.
- **Requires**: `INTEGRALL_ENABLE_SENSORS` + `INTEGRALL_ENABLE_LCD` (or OLED)
```cpp
void setup() {
    integrall.begin();
    // parkingSetup(trigPin, echoPin, warningCM, stopCM)
    integrall.parkingSetup(2, 0, 50.0, 15.0);
}

void loop() {
    integrall.parkingUpdate(); // Automatically displays distance and status
    integrall.handle();
}
```

### 🌡️ Weather Station
Monitors temperature and humidity with automatic LCD updates and IoT logging.
- **Requires**: `INTEGRALL_ENABLE_SENSORS` (with DHT library)
```cpp
void setup() {
    integrall.begin();
    // weatherSetup(dhtPin, dhtType, intervalSeconds)
    integrall.weatherSetup(4, 22, 30); 
}

void loop() {
    integrall.weatherUpdate(); // Reads on interval, updates LCD, sends to cloud
    integrall.handle();
}
```

### 💡 Smart Switch
Motion-activated lighting with automatic timeout.
- **Requires**: `INTEGRALL_ENABLE_RELAY` + `INTEGRALL_ENABLE_SENSORS`
```cpp
void setup() {
    integrall.begin();
    int light = integrall.enableRelay(13, true, "Light");
    // smartSwitchSetup(relayIndex, pirPin, autoOffSeconds)
    integrall.smartSwitchSetup(light, 19, 60); 
}

void loop() {
    integrall.smartSwitchUpdate(); // Auto-on with motion, auto-off after 60s
    integrall.handle();
}
```


---

## 🚀 The Universal IoT Foundation
Integrall is designed to be the **"OS" for your project**. Instead of just giving you "canned" projects like a Lock or Alarm, it provides a foundation that simplifies **every** line of code you write, whether you are building a Robot, a Bluetooth device, or an AI monitor.

### 1. Replaces the "Millis() Headache"
Most IoT projects break when you use `delay()`. Integrall provides non-blocking logic as a standard:
- ❌ **Standard**: `if (millis() - pM > interval) { ... }`
- ✅ **Integrall**: `integrall.blink(pin, interval);` or `integrall.sweepServo(pin, speed);`

### 2. Standardized Cloud bridge for ANY data
You don't need a special Integrall library for your sensor to send it to the cloud.
```cpp
// Works for any variable or sensor!
StaticJsonDocument<64> data;
data["co2"] = mySpecialSensor.read(); 
integrall.sendTelemetry(data);
```

### 3. The Responsive Background
Normal WiFi code makes your loop "stutter." Integrall's `handle()` runs in background "slices," ensuring your physical buttons and local logic stay lightning-fast while the network stays connected.

### 4. Bridge Any Input (Bluetooth, Serial, Web)
You can use Integrall modules to respond to any external trigger:
```cpp
if (Bluetooth.available()) {
   integrall.buzzerAlert(); // One line for a complex beep pattern
}
```

---
---

## 🌐 The Python Backend & Security

Integrall includes a professional-grade **FastAPI** backend that acts as the "Brain" for your devices. 

### 1. How the Backend Works
The backend creates a central point for all your ESP32s. Instead of you connecting directly to each ESP32, you connect to the **Backend**, and the backend manages the devices for you.

*   **Port 5000/8000**: The backend runs on your computer or a server.
*   **Database**: Every device registration, sensor reading, and command is logged in `integrall.db`.

### 2. The API Key (X-API-Key)
Security is built into the core. Every request the ESP32 makes must include a valid **API Key** in the header. If the key doesn't match, the server will reject the request with a `401 Unauthorized` error.

**Setup Security**:
1.  **Backend side**: Edit `backend/.env` and set `API_KEY=your_secret_string`.
2.  **Firmware side**: Set `config.api_key = "your_secret_string"` in your sketch.

### 3. Enabling the Connection
To keep the framework lightweight, backend synchronization is **OPTIONAL**. 

To enable it, you must toggle the flag in `Integrall/src/core/DeviceManager.h`:
```cpp
#define INTEGRALL_BACKEND_ENABLED 1  // Set to 1 to enable cloud sync
```

### 4. Basic Backend Sketch
```cpp
#define INTEGRALL_ENABLE_CAMERA
#include <Integrall.h>

void setup() {
    Integrall::DeviceConfig config;
    config.wifi_ssid     = "WiFi_Name";
    config.wifi_password = "Password";
    config.backend_url   = "http://192.168.1.XX:5000"; // Your PC's IP
    config.api_key       = "my_key_123";
    
    integrall.begin(config);
    integrall.enableCamera();
}

void loop() {
    integrall.handle(); // This pulls commands and sends data to backend
}
```

---

## 🧠 Core Engineering: How it works "Under the Hood"

Integrall uses a professional-grade **DeviceManager** to handle the complex networking tasks that usually make IoT development difficult.

### 1. Zero-Config Identity
The framework automatically generates a unique `Device ID` for you based on your ESP32's hardware MAC address (e.g., `INT_A1B2C3D4E5F6`). 
- **User benefit**: You don't have to manually name every device. Just plug it in, and it appears on your dashboard with its own identity.

### 2. The Smart State Machine
Integrall automatically moves your device through several states without you writing any code:
1. **WIFI_STA**: Reaches out to your router.
2. **REGISTERING**: Handshakes with your FastAPI server to introduce itself.
3. **ONLINE**: Fully ready and listening for remote commands.
4. **RECONNECTING**: If your router restarts, Integrall automatically detects the "Disconnected" event and tries to reconnect every 10 seconds.

### 3. Background Polling
In the `integrall.handle()` function, the framework periodically "asks" the backend if there are any commands (like "Turn Light ON"). 
- **Efficiency**: It uses a non-blocking timer, meaning your physical button presses and sensor readings are never slowed down by network activity.

---

## 🧪 Quick Reference: One-Liners vs Standard
| Feature | Standard Arduino | Integrall Way |
| :--- | :--- | :--- |
| **LCD Setup** | 5 lines (Wire, addr, init, backlight) | `integrall.begin();` |
| **Pot to Servo** | 3 lines (`analogRead`, `map`, `write`) | `integrall.setServoFromAnalog(9, A0);` |
| **Ultrasonic** | ~15 lines (pulsem, timing math) | `integrall.readDistance(5, 6);` |
| **Blink LED** | 4 lines + `delay(1000)` (Freezes) | `integrall.blink(2, 500);` |
| **Safely Delay** | `delay(1000)` (Freezes everything) | *Automatically Non-blocking* |

---

## 🐞 Support & Troubleshooting
1. **Highlighting**: If your code is all black, make sure you have the `keywords.txt` file in your library folder.
2. **Library Missing**: If `ArduinoJson.h` is missing, use the Library Manager to install it.
3. **ESP32 Issues**: Make sure to install the `ESP32Servo` library for the best performance.
4. **Missing Libraries**: Refer to [DEPENDENCIES.md](DEPENDENCIES.md) for a complete list of required and recommended libraries for each module.
