# Integrall Framework: User Guide

Welcome to the **Integrall** documentation. This guide covers every module and API available in the framework.

---

## Getting Started (The "Integrall" Way)
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

### 3. Smart Printing
Tired of `Serial.print` and `Serial.println` spanning over 5 lines just to print a variable? Use Integrall's C-style formatting! It only runs if your Debug Level is > 0.
```cpp
int angle = 90;
integrall.println("Moving the servo to exactly %d degrees!", angle);
```


---

## Connectivity & Cloud

Integrall handles the complexity of WiFi reconnections. You can provide WiFi credentials in two ways:

### 1. Global (The "Set and Forget" way)
Best for regular development. Edit `Integrall/src/config/IntegrallConfig.h`:
```cpp
#define INTEGRALL_DEFAULT_SSID     "YourWifiName"
#define INTEGRALL_DEFAULT_PASS     "YourWifiPassword"
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

    integrall.begin(config);
}
```

> **IMPORTANT:** Manual Config still requires `#define INTEGRALL_ENABLE_WIFI` at the top of your sketch to activate the networking hardware!

---

## Hardware Modules

### 1. LCD Display (I2C)
Hides all the `Wire.h` and address management, while offering dynamic features.
- **Setup**: `#define INTEGRALL_ENABLE_LCD`
- **Commands**:
  - `integrall.lcdPrint("Text", col, row);` - Direct printing. If you send a long string (e.g., >16 chars), Integrall will **automatically** scroll it for you.
  - `integrall.lcdClear();` - Clear the screen.
  - `integrall.lcdScrollText("Long Text", row, speed_ms);` - Force professional non-blocking scrolling on a specific row.
  - `integrall.lcdCursor(true, true);` - Show a blinking cursor.
  - `integrall.lcdCreateChar(0, myHeartArr);` - Easily draw custom symbols like batteries or hearts.
  - `integrall.lcdBacklight(true);` - Control backlight on/off.

### 2. Servo Motors
Works identically on **ESP32** and **Arduino Uno/Nano** (auto-swaps hardware timers). Supports up to 4 concurrent servos.
- **Setup**: `#define INTEGRALL_ENABLE_SERVO`
- **Commands (Single Servo)**:
  - `integrall.enableServo(pin, start_angle);` - Attach the physical GPIO pin, and optionally set a starting angle instantly.
  - `integrall.setServo(angle);` - Move motor precisely to angle (0-180) instantly.
  - `integrall.easeServo(angle, delay_ms);` - Super smooth, non-blocking ease to an angle. Motor slowly rotates in the background while your code keeps running.
  - `integrall.setServoFromAnalog(analog_pin);` - The "Potentiometer Trick". Automatically maps an analog twist knob (0-100%) to motor angle.
  - `integrall.sweepServo(speed_ms);` - **Non-blocking** sweep. Put this in your `loop()` and the motor automatically sweeps back and forth.

- **Commands (Multi-Servo)**:
  If you have more than one motor, `enableServo` returns an ID/index. You can use this ID as the **FIRST parameter** in any servo command to control them independently.
  ```cpp
  int arm = integrall.enableServo(10);
  int claw = integrall.enableServo(11);

  integrall.setServo(arm, 90);
  integrall.easeServo(claw, 180, 15);
  ```

### 3. Sensors (Ultrasonic, PIR, Analog)
No more complex math or timing logic.
- **Setup**: `#define INTEGRALL_ENABLE_SENSORS`
- **Smart Features**:
  - `float cm = integrall.readDistance(trig, echo, samples);` - Measures distance. The `samples` parameter (default 3) automatically filters out noise for a stable reading.
  - `bool near = integrall.isNear(trig, echo, threshold_cm);` - One-liner proximity alert. Returns `true` if an object is closer than the threshold.
  - `int light = integrall.readLightPercent(A0);` - Read LDR light intensity.
  - `int raw = integrall.readAnalogPercent(A0);` - Returns a clean 0-100% value from any analog sensor.
  - `bool motion = integrall.motionDetected(PIR_PIN);` - Semantic motion sensing.
  - `float waterTemp = integrall.readProbeTemp(PROBE_PIN);` - Read DS18B20 waterproof sensors. *(Requires DallasTemperature library)*
  - `float t, h, p; integrall.readEnvironment(addr, t, h, p);` - High-precision BME280 monitoring. *(Requires Adafruit_BME280 library)*
  - `float t = integrall.readTemperature(pin, type);` - DHT temperature reading. *(Requires DHT library)*
  - `float h = integrall.readHumidity(pin, type);` - DHT humidity reading. *(Requires DHT library)*

### 4. Smart Relays
Standard relay control with professional safety features.
- **Setup**: `#define INTEGRALL_ENABLE_RELAY`
- **Key Features**:
  - `integrall.enableRelay(pin, active_low, "Name");` - Initialize a relay. Returns an index for subsequent operations.
  - `integrall.relayOn(index);` / `integrall.relayOff(index);` / `integrall.relayToggle(index);` - Control.
  - `integrall.setRelay(index, true, alert);` - Set relay state directly, with optional buzzer alert.
  - `integrall.relaySetTimeout(index, 30000);` - **Auto-off** safety timer (30 seconds).
  - `integrall.relaySetInterlock(index, group);` - Prevent two relays in the same group turning on simultaneously (safety).
  - `integrall.relayAttachButton(index, pin);` - Attach a physical button to a relay (hardware override).
  - `integrall.relayIsOn(index);` - Check relay state.
  - `integrall.allRelaysOff();` - Emergency off for all relays.

### 5. Matrix Keypad
Hides row/column scanning and debounce logic completely.
- **Setup**: `#define INTEGRALL_ENABLE_KEYPAD`
- **Core Commands**:
  - `integrall.enableKeypad(rowPins, colPins);` - Initialize. Works with 4x4 or 4x3 keypads.
  - `char k = integrall.keypadGetKey();` - Read the currently pressed key (raw).
  - `integrall.keypadCapture(maxLen);` - Build a string from keypresses. `*` = Backspace, `#` = Enter.
  - `integrall.keypadCheckPin("1234");` - Validate string against a PIN. Clears buffer automatically.
  - `integrall.keypadClear();` - Clear the input buffer.

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
  - `integrall.buzzerPattern(count, on_ms, off_ms);` - Custom beep pattern.
  - `integrall.buzzerAlert();` - Triple rapid beep for danger/alarm.
  - `integrall.buzzerSuccess();` / `integrall.buzzerFail();` - Melodic feedback.
  - `integrall.buzzerOff();` - Silence.

### 8. RGB LED
Control multi-color LEDs with named colors or RGB values.
- **Setup**: `#define INTEGRALL_ENABLE_RGB`
- **Commands**:
  - `integrall.enableRGB(rPin, gPin, bPin, commonAnode);` - Initialize.
  - `integrall.setColor("blue");` - Set by name (red, green, blue, yellow, cyan, white, purple, orange).
  - `integrall.setRGB(r, g, b);` - Custom color balance.
  - `integrall.rgbBlink(r, g, b, ms);` - **Non-blocking** color blink.
  - `integrall.rgbOff();` - Turn off.

### 9. ESP32-CAM Video Stream
Professional MJPEG streaming with zero boilerplate.
- **Setup**: `#define INTEGRALL_ENABLE_CAMERA`
- **Requires**: `#define INTEGRALL_ENABLE_WIFI` (Camera will be disabled without WiFi)
- **Pick your Board**: Define your model at the **very top** of your sketch (e.g., `#define CAMERA_MODEL_AI_THINKER`). If none is defined, AI-Thinker is used by default.
- **Supported Models**: AI-Thinker, Wrover Kit, ESP-EYE, M5Stack (all versions), TTGO T-Journal, Xiao ESP32S3, DFRobot FireBeetle2/Romeo, and more.
- **Commands**:
  - `integrall.enableCamera();` - Inits hardware and **auto-starts** a web server on Port 81 as soon as WiFi connects.
  - `const char* url = integrall.getCameraStreamURL();` - Get the link to your live video feed.
  - `integrall.startCameraServer();` - Manually start/restart the server if needed.

**Smart Memory Management**: Integrall automatically detects if your board has **PSRAM**. If it does, it enables High-Quality UXGA (1600x1200) streaming. If not, it safely downscales to prevent crashes.

### 10. Non-Blocking Blinker
The ultimate replacement for the "Blink" example. No `delay()` needed. Always available (no enable flag required).
- **Commands**:
  - `integrall.blink(pin, interval);` - Start blinking in the background.
  - `integrall.stopBlink();` - Stop the blinking.

**Why use this?** Traditional `delay(1000)` freezes your ESP32. With `integrall.blink()`, your LED flashes while your WiFi and sensors keep working perfectly.

### 11. Rich Audio (MP3 & Voice)
Play high-fidelity sound effects or voice alerts via MicroSD.
- **Setup**: `#define INTEGRALL_ENABLE_AUDIO`
- **Commands**:
  - `integrall.audioBeginDF(Serial2, rx, tx);` - Start DFPlayer on Serial2. *(Requires DFRobotDFPlayerMini library)*
  - `integrall.audioPlay(track);` - Start specific MP3.
  - `integrall.audioVolume(25);` - Set volume (0-30).
  - `integrall.audioSoundLevel(pin);` - Read microphone/sound level (ESP32 only).

### 12. Input UI & Auth
Unified interaction for buttons, encoders, joysticks, and more.
- **Setup**: `#define INTEGRALL_ENABLE_INPUT`
- **Commands**:
  - `integrall.inputButtonPressed(pin, activeLow);` - Debounced button press.
  - `integrall.inputReadJoystick(pin, center, deadzone);` - Normalized joystick axis (-100 to 100).
  - `integrall.inputTouchActive(pin);` - Capacitive touch detection.
  - `integrall.inputReadEncoder(clk, dt);` - Rotary encoder direction.
  - `String card = integrall.inputReadRFID(ss, rst);` - Get MFRC522 UID. *(Requires MFRC522 library)*
  - `uint32_t code = integrall.inputReadIR(pin);` - Parse IR remote codes. *(Requires IRremote library)*

### 13. Storage & SD Logging
Persistent data capture for industrial or weather monitoring.
- **Setup**: `#define INTEGRALL_ENABLE_STORAGE`
- **Commands**:
  - `integrall.storageBeginEEPROM(size);` - Initialize EEPROM with given size.
  - `integrall.storageWriteInt(addr, val);` - Store setting in EEPROM.
  - `integrall.storageReadInt(addr);` - Read setting from EEPROM.
  - `integrall.storageLog("/log.txt", "Update", cs);` - Append to SD card file. *(Requires SD library)*
  - `integrall.storageDelete("/log.txt");` - Delete SD card file. *(Requires SD library)*

### 14. Time & GPS
NTP synchronization and RTC support.
- **Setup**: `#define INTEGRALL_ENABLE_TIME`
- **Commands**:
  - `integrall.timeSetupNTP("pool.ntp.org", gmt, dst);` - Sync clock via internet (ESP32/ESP8266 only).
  - `integrall.timeIsSynced();` - Check if NTP sync is done (ESP32/ESP8266 only).
  - `integrall.timeBeginRTC();` - Initialize DS3231 RTC. *(Requires RTClib)*
  - `String t = integrall.timeGetStr();` - Get current HH:MM:SS from RTC. *(Requires RTClib)*
  - `String iso = integrall.timeGetISO();` - Get ISO 8601 datetime from RTC. *(Requires RTClib)*
  - `float lat, lng; integrall.timeGetGPS(Serial2, lat, lng);` - Real-time location. *(Requires TinyGPS++ library)*

### 15. Wireless (BLE / LoRa)
Go beyond WiFi for long-range or mobile-to-device communication.
- **Setup**: `#define INTEGRALL_ENABLE_COMM`
- **Commands**:
  - `integrall.commBeginBLE("MyNode");` - Start Bluetooth Advertising. *(Requires BLE libraries, ESP32 only)*
  - `integrall.commUpdateBLE(value);` - Update BLE characteristic value.
  - `integrall.commBeginLoRa(868E6);` - Start Radio transmission. *(Requires LoRa library)*
  - `integrall.commPushLoRa("payload");` - Send LoRa packet.

### 16. Power & Battery
Monitor energy budget and optimize for longevity.
- **Setup**: `#define INTEGRALL_ENABLE_POWER`
- **Commands**:
  - `integrall.powerBeginINA(address);` - Initialize INA219 current sensor. *(Requires Adafruit_INA219 library)*
  - `float v = integrall.powerGetVoltage();` - Bus voltage.
  - `float ma = integrall.powerGetCurrent();` - Current draw.
  - `float mw = integrall.powerGetPower();` - Power consumption.
  - `int pct = integrall.powerGetBattery(voltage);` - Battery percentage estimate.
  - `integrall.powerDeepSleep(600);` - Enter deep sleep for 10 minutes.

### 17. Stepper Control
Precise 4-wire motor rotation.
- **Setup**: `#define INTEGRALL_ENABLE_STEPPER`
- **Commands**:
  - `integrall.stepperBegin(stepsPerRev, p1, p2, p3, p4);` - Initialize motor.
  - `integrall.stepperSpeed(rpm);` - Set rotation speed.
  - `integrall.stepperStep(steps);` - Move by step count.
  - `integrall.stepperMove(degrees);` - Rotate by degrees.

---

## High-Level Project Systems

### PIN Lock System
The most powerful feature of Integrall — a complete PIN-code lock system in **2 function calls**.

**Requires**: `INTEGRALL_ENABLE_KEYPAD` + `INTEGRALL_ENABLE_RELAY`
**Optional**: Add `INTEGRALL_ENABLE_LCD` for automatic feedback messages.

#### What it handles automatically:
| Feature | Automatic |
| :--- | :---: |
| Asterisk display as user types | Yes |
| Backspace (`*`) and Submit (`#`) | Yes |
| "Unlocked" / "Access Denied" messages | Yes |
| Remaining attempts counter on LCD | Yes |
| Lockout after N wrong attempts | Yes |
| Auto-lock relay after unlock duration | Yes |

#### Setup (one call in `setup()`):
```cpp
// lockSetup(pin, relayIndex, unlockDuration_ms, maxWrongAttempts)
integrall.lockSetup("1234", door, 5000, 3);
```

#### Run (one call in `loop()`):
```cpp
integrall.lockUpdate(); // Handles EVERYTHING
```

#### Reset lockout:
```cpp
integrall.lockReset(); // Clears attempt counter and lockout state
```

#### Complete Sketch:
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

### Alarm System
A professional security system with PIR motion detection, cooldown safety, and sound/light alerts.
- **Requires**: `INTEGRALL_ENABLE_SENSORS` + `INTEGRALL_ENABLE_RELAY`
- **Optional**: `INTEGRALL_ENABLE_BUZZER` for audio alerts, `INTEGRALL_ENABLE_LCD` for status display
```cpp
void setup() {
    integrall.begin();
    int siren = integrall.enableRelay(13, true, "Siren");
    // alarmSetup(pirPin, relayIndex, cooldownMs)
    integrall.alarmSetup(19, siren, 10000);
}

void loop() {
    integrall.alarmUpdate(); // Automatically manages detection & triggers
    integrall.handle();
}
```

### Parking Sensor
Visual and audible distance assistance for garages or robots.
- **Requires**: `INTEGRALL_ENABLE_SENSORS`
- **Optional**: `INTEGRALL_ENABLE_LCD` for distance display, `INTEGRALL_ENABLE_BUZZER` for proximity beeps
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

### Weather Station
Monitors temperature and humidity with automatic LCD updates.
- **Requires**: `INTEGRALL_ENABLE_SENSORS` (with DHT library installed)
- **Optional**: `INTEGRALL_ENABLE_LCD` for live readings display
```cpp
void setup() {
    integrall.begin();
    // weatherSetup(dhtPin, dhtType, intervalSeconds)
    integrall.weatherSetup(4, 22, 30);
}

void loop() {
    integrall.weatherUpdate(); // Reads on interval, updates LCD
    integrall.handle();
}
```

### Smart Switch
Motion-activated lighting with automatic timeout.
- **Requires**: `INTEGRALL_ENABLE_RELAY` + `INTEGRALL_ENABLE_SENSORS`
- **Optional**: `INTEGRALL_ENABLE_LCD` for status display
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

## The Universal IoT Foundation
Integrall is designed to be the **"OS" for your project**. Instead of just giving you "canned" projects like a Lock or Alarm, it provides a foundation that simplifies **every** line of code you write, whether you are building a Robot, a Bluetooth device, or an AI monitor.

### 1. Replaces the "Millis() Headache"
Most IoT projects break when you use `delay()`. Integrall provides non-blocking logic as a standard:
- **Standard**: `if (millis() - pM > interval) { ... }`
- **Integrall**: `integrall.blink(pin, interval);` or `integrall.sweepServo(speed);`

### 2. The Responsive Background
Normal WiFi code makes your loop "stutter." Integrall's `handle()` runs in background "slices," ensuring your physical buttons and local logic stay lightning-fast while the network stays connected.

### 3. Bridge Any Input (Bluetooth, Serial, Web)
You can use Integrall modules to respond to any external trigger:
```cpp
if (Bluetooth.available()) {
   integrall.buzzerAlert(); // One line for a complex beep pattern
}
```

---

## Networking & Utility Methods

These are available when WiFi is enabled (`#define INTEGRALL_ENABLE_WIFI`):

- `integrall.isOnline()` - Check if system is fully online.
- `integrall.isWiFiConnected()` - Check WiFi connection status.
- `integrall.getDeviceId()` - Get unique device ID (based on MAC address).
- `integrall.getIPAddress()` - Get assigned IP address.
- `integrall.getWiFiStrength()` - Get WiFi signal strength (RSSI).
- `integrall.getStatusString()` - Get current system state as string.
- `integrall.reconnect()` - Force reconnection.
- `integrall.httpGet(url)` - Perform HTTP GET request.
- `integrall.httpPost(url, payload)` - Perform HTTP POST request.
- `integrall.enableEventLog(enabled)` - Enable/disable automatic event logging.

---

## Cloud Backend (Under Development)

> **NOTE:** The Python/FastAPI cloud backend is currently **under active development** and is not yet included in the distribution. The firmware-side networking code (WiFi, device registration, telemetry) is fully implemented and functional — but there is no backend server available to connect to at this time.
>
> Planned backend features include:
> - Device registration and management dashboard
> - Telemetry data collection and storage
> - Remote command dispatch
> - API key authentication
>
> When the backend is ready, the firmware's existing `sendTelemetry()`, `httpGet()`, and `httpPost()` methods will connect seamlessly. No firmware changes will be required.

---

## Quick Reference: One-Liners vs Standard
| Feature | Standard Arduino | Integrall Way |
| :--- | :--- | :--- |
| **LCD Setup** | 5 lines (Wire, addr, init, backlight) | `integrall.begin();` |
| **Pot to Servo** | 3 lines (`analogRead`, `map`, `write`) | `integrall.setServoFromAnalog(A0);` |
| **Ultrasonic** | ~15 lines (pulse, timing math) | `integrall.readDistance(5, 6);` |
| **Blink LED** | 4 lines + `delay(1000)` (Freezes) | `integrall.blink(2, 500);` |
| **Safely Delay** | `delay(1000)` (Freezes everything) | *Automatically Non-blocking* |

---

## Support & Troubleshooting
1. **Highlighting**: If your code is all black, make sure you have the `keywords.txt` file in your library folder.
2. **Library Missing**: If `ArduinoJson.h` is missing, use the Library Manager to install it.
3. **ESP32 Issues**: Make sure to install the `ESP32Servo` library for the best performance.
4. **Missing Libraries**: Refer to [DEPENDENCIES.md](DEPENDENCIES.md) for a complete list of required and recommended libraries for each module.
