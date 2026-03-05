# 📖 Integrall Framework: User Guide

Welcome to the **Integrall** documentation. This guide is updated in real-time as the framework evolves to help you build fast, safe, and powerful IoT projects.

---

## 🛠️ Getting Started (The "Integrall" Way)
The philosophy of Integrall is **Complexity Abstraction**. We hide the "ugly" setup so you can write logic in one line.

### 1. Minimal Sketch Structure
Unlike normal Arduino code, you define what you need **BEFORE** including the library.

```cpp
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_LCD
#include <Integrall.h>

Integrall::System integrall;

void setup() {
    integrall.begin(); // Automatically handles I2C, Serial, and internal buffers
}

void loop() {
    integrall.handle(); // Keeps background tasks (WiFi, timers) running
}
```

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
  - `int light = integrall.readAnalogPercent(A0);` - Returns a clean 0-100% value from any analog sensor.
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

> **Tip**: To reset a lockout from code, call `integrall.lockReset();`

---

## 🌐 IoT & Backend
Integrall is built for the cloud. The ESP32 automatically syncs with your server.

### Automatic Status Engine
If you have an LCD enabled, Integrall **automatically** monitors your connection.
- If WiFi drops, the LCD will flash: `WiFi Discon...`
- If the Server is down, the LCD will flash: `Backend Error`

### Sending Telemetry
Send sensor data to your dashboard with one command:
```cpp
StaticJsonDocument<128> doc;
doc["temp"] = integrall.readAnalogPercent(A0);
integrall.sendTelemetry(doc);
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
| **Safely Delay** | `delay(1000)` (Freezes everything) | *Use Non-blocking modules* |

---

## 🐞 Support & Troubleshooting
1. **Highlighting**: If your code is all black, make sure you have the `keywords.txt` file in your library folder.
2. **Library Missing**: If `ArduinoJson.h` is missing, use the Library Manager to install it.
3. **ESP32 Issues**: Make sure to install the `ESP32Servo` library for the best performance.
