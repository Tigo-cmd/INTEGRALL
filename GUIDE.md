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
  - `integrall.setServoFromAnalog(servo_pin, pot_pin);` - Automatically maps a potentiometer to the motor.
  - `integrall.sweepServo(pin, speed);` - **Non-blocking** sweep (doesn't freeze your code!).

### 3. Sensors (Ultrasonic, PIR, Analog)
No more complex math or timing logic.
- **Setup**: `#define INTEGRALL_ENABLE_SENSORS`
- **Commands**:
  - `float cm = integrall.readDistance(trig, echo);` - Measures distance in CM.
  - `int light = integrall.readAnalogPercent(A0);` - Returns a clean 0-100% value.
  - `bool motion = integrall.isTriggered(PIR_PIN);` - Simple true/false trigger.

### 4. Smart Relays
Standard relay control with professional safety features.
- **Setup**: `#define INTEGRALL_ENABLE_RELAY`
- **Key Features**:
  - `integrall.enableRelay(pin, active_low, "Name");` - Initialize a relay.
  - `integrall.relaySetTimeout(index, 30000);` - **Auto-off** safety timer (30 seconds).

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
