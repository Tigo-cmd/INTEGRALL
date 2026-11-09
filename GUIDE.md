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
    integrall.begin("SecretNetwork", "Password123", "http://your-backend.com");
}
```

### 3. Access Point (AP) Mode
If you want the ESP32 to create its own WiFi network so you can connect to it directly from your phone.
```cpp
void setup() {
    integrall.beginAP("Integrall hotspot", "password123");
}
```

> **IMPORTANT:** All WiFi modes still require `#define INTEGRALL_ENABLE_WIFI` at the top of your sketch!

---

## Connectivity & Cloud

Integrall handles the complexity of WiFi reconnections and backend synchronization automatically.

### Sending Data to Cloud (Telemetry)
Once the system is `ONLINE` (after `begin()` with a backend URL), you can send data with zero JSON boilerplate:
```cpp
void loop() {
    integrall.handle();
    
    // Send a single value
    integrall.sendTelemetry("temperature", 24.5);
    
    // Or send a bundle of data
    JsonDocument doc;
    doc["temp"] = 25.0;
    doc["hum"] = 60;
    integrall.sendTelemetry(doc);
}
```

### Checking Status
- `integrall.isOnline()`: Returns `true` if connected to WiFi AND the backend.
- `integrall.isWiFiConnected()`: Returns `true` if WiFi is up.
- `integrall.getIPAddress()`: Returns the local IP.

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
  If you have more than one motor, `enableServo` returns an ID/index. Pass the ID as the **FIRST parameter** together with the remaining parameters (`setServo(id, angle)`, `sweepServo(id, speed_ms)`, `easeServo(id, angle, speed_ms)`) to control them independently.
  ```cpp
  int arm = integrall.enableServo(10);
  int claw = integrall.enableServo(11);

  integrall.setServo(arm, 90);
  integrall.sweepServo(arm, 15);
  integrall.easeServo(claw, 180, 15);
  ```

### 3. Universal Sensor & IMU Engine (30+ Sensors)
No more complex math, calibration curves, or timing logic.
- **Setup**: `#define INTEGRALL_ENABLE_SENSORS` (and `#define INTEGRALL_ENABLE_IMU` for MPU6050)
- **Temperature & Climate**:
  - `float t = integrall.readTemperature(pin, type);` / `integrall.readHumidity(pin, type);` - DHT11/DHT22.
  - `float waterTemp = integrall.readProbeTemp(pin);` - DS18B20 waterproof probe.
  - `float t, h, p; integrall.readEnvironment(addr, t, h, p);` - BME280 (Temp, Humidity, Pressure hPa).
  - `float p = integrall.readPressure(addr);` - BMP280 / BME280 pressure in hPa.
  - `float t, h; integrall.readSHT3x(0x44, t, h);` - Native I2C SHT30/SHT31/SHT35 (no external library needed).
  - `float t = integrall.readThermistorC(pin, beta, r0, seriesR);` - NTC Thermistor (Steinhart-Hart Beta equation).
  - `float t = integrall.readLM35C(pin);` / `integrall.readTMP36C(pin);` - Linear analog temperature sensors.
  - `integrall.calculateDewPoint(t, h);` / `integrall.calculateHeatIndex(t, h);` / `integrall.calculateAltitude(p);` - Psychrometric math.
- **Light, Lux, UV & Color**:
  - `int light = integrall.readLightPercent(pin, reverse);` - LDR light intensity (0-100%).
  - `float lux = integrall.readLux(0x23);` - BH1750 digital ambient light sensor in Lux (native I2C).
  - `float uv = integrall.readUVIndex(pin);` - GUVA-S12SD / ML8511 UV Index (0.0-15.0).
  - `uint8_t r, g, b; integrall.readColorRGB(s2, s3, out, r, g, b);` - TCS3200/TCS230 RGB color sensor.
- **Distance, IR & Laser Time-of-Flight (ToF)**:
  - `float cm = integrall.readDistanceCM(trig, echo, samples);` - Multi-sample filtered HC-SR04 ultrasonic.
  - `bool near = integrall.isNear(trig, echo, threshold_cm);` / `integrall.isWithinRange(trig, echo, min_cm, max_cm);`
  - `float irCm = integrall.readIRDistanceCM(pin);` - Sharp GP2Y0A21 analog IR distance (10-80 cm).
  - `int mm = integrall.readLaserDistanceMM();` - VL53L0X Laser ToF distance in millimeters.
- **Air Quality, Gas, PM2.5 Dust & CO2**:
  - `int pct = integrall.readAnalogPercent(pin);` - Raw 0-100% analog level.
  - `float ppm = integrall.readGasPPM(pin, r0, rl, a, b);` - Calibrated MQ-2/4/7/135 gas concentration in PPM.
  - `float ug = integrall.readDustDensity(ledPin, analogPin);` - Sharp GP2Y1010AU0F PM2.5 optical dust density (µg/m³).
  - `int co2 = integrall.readCO2PWM(pwmPin, 5000);` - MH-Z19B NDIR CO2 concentration (ppm).
- **Soil, Agriculture, Water Quality & Liquid Flow**:
  - `int soil = integrall.readSoilMoisture(pin, dryRaw, wetRaw);` - Calibrated capacitive/resistive soil moisture (0-100%).
  - `int rain = integrall.readRainPercent(pin);` / `bool wet = integrall.isRaining(pin, 30);` - Rain & water leak detection.
  - `float tds = integrall.readTDS(pin, waterTempC);` - Temperature-compensated TDS water purity (ppm).
  - `float ph = integrall.readPH(pin);` - Analog pH probe (0.0-14.0 pH).
  - `float ntu = integrall.readTurbidityNTU(pin);` - Water cloudiness in NTU (0-3000).
  - `float lpm = integrall.readWaterFlowLPM(pin, 7.5);` - YF-S201 turbine water flow rate (L/min).
- **Weight (HX711), Current (ACS712), Voltage & Heart Pulse**:
  - `long raw = integrall.readWeightRaw(dout, sck);` / `float g = integrall.readWeightGrams(dout, sck, tare, scale);` - Native 24-bit HX711 load cell driver.
  - `float aDC = integrall.readCurrentDC(pin, 185.0);` / `float aAC = integrall.readCurrentAC(pin, 185.0);` - ACS712 DC and AC RMS current (Amps).
  - `float v = integrall.readVoltageDivider(pin, 5.0);` - DC voltage sensor (0-25V divider).
  - `int pulse = integrall.readHeartPulsePercent(pin);` - Analog PPG heart pulse sensor.
- **Security Triggers & 6-Axis IMU (MPU6050)**:
  - `integrall.motionDetected(pin)`, `flameDetected(pin)`, `doorOpen(pin)`, `vibrationDetected(pin)`, `soundDetected(pin)`, `tiltDetected(pin)`, `magneticDetected(pin)`, `touchDetected(pin)`.
  - `integrall.enableIMU(sda, scl)`, `getImpactForce()`, `getPitch()`, `getRoll()`, `getPosture()`, `isHorizontal()`, `getIMUTemperature()`.

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
  - `char last = integrall.keypadLastKey();` - Read the most recently pressed key.
  - `integrall.keypadCapture(maxLen);` - Build a string from keypresses. `*` = Backspace, `#` = Enter.
  - `integrall.keypadSubmitted();` - Returns `true` when `#` is pressed with a non-empty buffer.
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

### 9. Universal ESP32-CAM Vision & Video Stream
Professional MJPEG streaming and AI snapshot capture across **19+ ESP32 / ESP32-S2 / ESP32-S3 camera boards** and **multiple ribbon sensors (`OV2640`, `OV3660`, `OV5640`, `GC2145`)** with zero boilerplate.
- **Setup**: `#define INTEGRALL_ENABLE_CAMERA`
- **Requires**: `#define INTEGRALL_ENABLE_WIFI`
- **Supported Carrier Boards (`CameraBoard` Enum)**:
  - `CAM_BOARD_AI_THINKER` (Default AI-Thinker ESP32-CAM)
  - `CAM_BOARD_XIAO_ESP32S3` (Seeed Studio XIAO ESP32S3 Sense)
  - `CAM_BOARD_FREENOVE_S3` (Freenove ESP32-S3-WROOM CAM)
  - `CAM_BOARD_WROVER_KIT` (ESP-WROVER-KIT / Freenove WROVER CAM)
  - `CAM_BOARD_ESP_EYE` / `CAM_BOARD_ESP32S3_EYE` (Espressif ESP-EYE & ESP32-S3-EYE)
  - `CAM_BOARD_M5STACK_PSRAM`, `CAM_BOARD_M5STACK_V2_PSRAM`, `CAM_BOARD_M5STACK_WIDE`, `CAM_BOARD_M5STACK_ESP32CAM`, `CAM_BOARD_M5STACK_UNITCAM`, `CAM_BOARD_M5STACK_CAMS3_UNIT`
  - `CAM_BOARD_TTGO_T_JOURNAL`, `CAM_BOARD_LILYGO_T_CAMERA_PLUS`, `CAM_BOARD_LILYGO_T_SIMCAM`
  - `CAM_BOARD_DFROBOT_FIREBEETLE2_S3`, `CAM_BOARD_ESP32_CAM_BOARD`, `CAM_BOARD_ESP32S2_CAM_BOARD`, `CAM_BOARD_ESP32S3_CAM_LCD`
  - Custom PCBs via `Integrall::CameraPins` struct
- **Automatic Ribbon Sensor Tuning**: Integrall automatically inspects the camera ribbon PID (`OV2640`, `OV3660`, `OV5640`, `GC2145`, etc.) after initialization and applies orientation (`vflip`/`hmirror`) and color/saturation corrections out of the box.
- **Commands**:
  - `integrall.setCameraBoard(CAM_BOARD_XIAO_ESP32S3);` - Optional pre-selection before `integrall.begin()`.
  - `integrall.enableCamera();` or `integrall.enableCamera(CAM_BOARD_XIAO_ESP32S3, FRAMESIZE_VGA, 10);` - Initialize (or re-configure) camera hardware for any board, resolution (`FRAMESIZE_QVGA`, `FRAMESIZE_VGA`, `FRAMESIZE_SVGA`, `FRAMESIZE_HD`, `FRAMESIZE_UXGA`), and JPEG quality (`4–63`).
  - `integrall.startCameraServer();` - Starts the control UI on Port 80 and MJPEG stream server on Port 81 (`/stream`).
  - `const char* url = integrall.getCameraStreamURL();` - Get the live video stream URL.
  - `camera_fb_t* fb = integrall.cameraCapture();` / `integrall.cameraRelease(fb);` - Capture and release a single JPEG frame.
  - `integrall.setCameraResolution(FRAMESIZE_SVGA);` / `integrall.setCameraQuality(10);` - Adjust resolution or compression on the fly.
  - `integrall.setCameraFlip(vflip, hmirror);` - Flip or mirror the image at runtime.
  - `integrall.setCameraBrightness(level);` / `setCameraContrast(level);` / `setCameraSaturation(level);` - Image tuning (`-2` to `+2`).
  - `integrall.setCameraEffect(effect);` / `integrall.setCameraWhiteBalance(enable, mode);` - Hardware special effects (`0–6`) and white balance modes (`0–4`).
  - `integrall.setCameraFlash(intensity);` - Control the board's Flash LED (`0–255` PWM brightness).
  - `integrall.getCameraBoardName();` / `integrall.getCameraSensorName();` - Inspect active board profile and detected ribbon sensor (`"OV2640"`, `"OV3660"`, `"OV5640"`, etc.).

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

## Networking, HTTPS REST Client, C++ HTML Renderer & WebSockets

These are available when WiFi is enabled (`#define INTEGRALL_ENABLE_WIFI`):

### 1. WiFi & Device Identity
- `integrall.isOnline()` - Check if system is fully online.
- `integrall.isWiFiConnected()` - Check WiFi connection status.
- `integrall.getDeviceId()` - Get unique device ID (based on MAC address).
- `integrall.getIPAddress()` - Get assigned IP address (or `192.168.4.1` in Hotspot mode).
- `integrall.getWiFiStrength()` - Get WiFi signal strength (RSSI).
- `integrall.getStatusString()` - Get current system state as string.
- `integrall.reconnect()` - Force reconnection.

### 2. HTTP & HTTPS REST Client (`GET`, `POST`, `PUT`, `PATCH`, `DELETE`, TLS, Auth)
Integrall automatically detects `https://` vs `http://` URLs and configures `WiFiClientSecure` out-of-the-box:
- `integrall.setBearerToken(token)` - Attach `Authorization: Bearer <token>` to all outgoing requests.
- `integrall.setHttpHeader(key, value)` / `integrall.clearHttpHeaders()` - Add up to 6 custom HTTP headers.
- `integrall.setCACert(rootCA)` / `integrall.setHttpInsecure(true)` - Pin a Root CA certificate or allow zero-config TLS.
- `String body = integrall.httpGet(url, optionalToken)` - HTTP/HTTPS `GET` request.
- `int code = integrall.httpPost(url, jsonPayload, contentType, optionalToken)` - HTTP/HTTPS `POST` request.
- `int code = integrall.httpPut(url, jsonPayload, contentType, optionalToken)` - HTTP/HTTPS `PUT` request.
- `int code = integrall.httpPatch(url, jsonPayload, contentType, optionalToken)` - HTTP/HTTPS `PATCH` request.
- `int code = integrall.httpDelete(url, optionalPayload, optionalToken)` - HTTP/HTTPS `DELETE` request.
- `Integrall::HttpResponse r = integrall.httpRequest(method, url, payload)` - Full response struct (`r.statusCode`, `r.body`, `r.contentLength`, `r.ok()`).
- `integrall.getLastStatusCode()` / `integrall.getLastResponse()` - Inspect last HTTP status code and response body.

### 3. C++ HTML Tag & Responsive IoT Dashboard Renderer (`HtmlBuilder`)
Render semantic HTML5 tags and responsive dark-mode IoT dashboards in C++ without raw string concatenation:
- `integrall.htmlTag("div", innerHtml, "class='box'")` / `integrall.htmlVoidTag("img", "src='/cam.jpg'")`
- `integrall.htmlCard(title, bodyHtml)` - Glassmorphic container card.
- `integrall.htmlMetric(label, value, unit, elementId)` - Live telemetry readout card (e.g., `"Temperature"`, `"25.4"`, `"°C"`, `"temp"`).
- `integrall.htmlBadge(text, color)` - Status pill (`"cyan"`, `"green"`, `"red"`, `"amber"`).
- `integrall.htmlButton(label, endpoint, color)` - Async `fetch(endpoint)` button (no page reload!).
- `integrall.htmlToggle(label, id, checked, endpoint)` - Interactive ON/OFF switch calling `endpoint?state=1|0`.
- `integrall.htmlSlider(label, id, min, max, value, endpoint)` - Interactive range slider calling `endpoint?val=X`.
- `integrall.htmlProgressBar(label, percent, id)` - Animated `0–100%` progress bar.
- `integrall.htmlWsScript(wsPort, targetId)` - Injects an auto-reconnecting browser WebSocket client that binds incoming JSON keys directly to DOM element IDs!
- `integrall.htmlPage(title, bodyHtml, autoRefreshSec, darkTheme)` - Wraps content in a complete responsive HTML5 document.

### 4. Local HTTP/REST Web Server (`Port 80`)
Host local web pages and REST API endpoints directly from the ESP32/ESP8266:
- `integrall.webServerBegin(80);`
- `integrall.servePage("/", fullHtml);` / `integrall.serveJson("/health", "{\"ok\":true}");`
- `integrall.onGet("/api/relay", []() { ... });` (also `onPost`, `onPut`, `onPatch`, `onDelete`)
- Inside route handlers:
  - `String v = integrall.webArg("state");` - Query parameter (`?state=1`) or URL-encoded form field.
  - `String b = integrall.webBody();` - Raw request body (e.g. JSON payload).
  - `integrall.webSendHtml(html)` / `integrall.webSendJson(json)` / `integrall.webSendText(text)` / `integrall.webRedirect(url)`

### 5. Native Zero-Dependency RFC 6455 WebSockets (Server & Client)
Real-time bidirectional messaging with zero external libraries required:
- **WebSocket Server**:
  - `integrall.wsServerBegin(81);`
  - `integrall.onWsMessage([](const char* msg) { ... });`
  - `integrall.wsBroadcast("{\"temp\":25.4}");`
  - `integrall.wsClientCount();`
- **WebSocket Client**:
  - `integrall.wsConnect("ws://192.168.1.50:8080/ws");`
  - `integrall.wsSend("{\"hello\":\"server\"}");`
  - `integrall.onWsClientMessage([](const char* msg) { ... });`
  - `integrall.wsConnected();` / `integrall.wsDisconnect();`

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
