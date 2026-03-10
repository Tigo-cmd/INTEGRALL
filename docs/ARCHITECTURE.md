# Integrall Framework – Architecture Specification

This document defines the deep internal structure and communication patterns for **Integrall**, a modern C++ abstraction framework for ESP32/ESP8266 IoT devices.

The goal of this architecture is to ensure that as we scale from 17 modules to 50+ modules, the framework never becomes a tangled, memory-leaking mess.

---

## 1. The Core Problem (Why Architecture Matters)

In traditional Arduino code, developers instantiate everything globally:
```cpp
LiquidCrystal_I2C lcd(0x27,16,2);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Servo myservo;
```
As projects grow, these global objects fight for interrupts, I2C buses, and SRAM, leading to unpredictable crashes.

**Integrall solves this** by centralizing hardware access using a **Singleton Manager Architecture**. The user only interacts with the top-level API (`Integrall::System`), which safely orchestrates the underlying modules.

---

## 2. High-Level Architecture (The Layer Cake)

Integrall is built in four distinct layers:

### Layer 1: Application Layer (The User Sketch)
This is what the user writes. It is clean, declarative, and contains zero setup boilerplate.
**Rule:** The user NEVER touches raw pointers, `Wire.h`, or `WiFi.h` directly.

### Layer 2: The `System` API (`Integrall.h`)
The "God Object" exposed to the user as `integrall`. It acts as a strict facade pattern. 
**Rule:** This layer contains NO hardware logic. It merely passes user commands down to the Module Managers.

### Layer 3: Module Managers (`DeviceManager`, `OLEDModule`, etc.)
Dedicated C++ classes that encapsulate and protect specific hardware protocols. 
**Rule:** Managers must initialize safely (checking if the bus is already active) and clean up after themselves to prevent memory leaks. 

### Layer 4: Foundation/Drivers (3rd Party Libs)
The raw driver code (e.g., `Adafruit_GFX`, `ArduinoJson`, `HTTPClient`).
**Rule:** These are kept completely isolated. A change in an Adafruit library should require a change *only* in the corresponding Integrall Module Manager, never in the top-level API.

---

## 3. The Central Loop Concept: `integrall.handle()`

The secret to Integrall's non-blocking magic is the `handle()` method. Because `delay()` freezes the ESP32 and drops WiFi connections, Integrall relies heavily on State Machines.

Every time `integrall.handle()` is called in the main loop, it fires a sequence:
1. `DeviceManager` checks WiFi. If dropped, it initiates a non-blocking reconnect via async timers.
2. `ServoModule` checks its easing buffers. If a servo is animating, it calculates the next microsecond pulse and updates the PWM.
3. `LCDModule` checks its scroll buffers. If text needs to shift, it shifts the characters exactly 1 pixel width.
4. `RelayModule` checks its safety timeouts. If a pump has been on for 60 seconds, it shuts it off.

This ensures that a single `handle()` call efficiently micro-manages the entire device in < 1ms.

---

## 4. Feature Flag System (Pay For What You Use)

Because we wrap massive libraries (`Adafruit_SSD1306`, `DFRobotDFPlayerMini`), compiling them all would instantly exhaust the ESP8266’s memory.

Integrall uses a strict **Pre-Processor Flag Architecture** (`#if defined(...)`).

```cpp
#if INTEGRALL_MODULE_OLED_ENABLED
    #include <Adafruit_GFX.h>
    #include <Adafruit_SSD1306.h>
    OLEDModule _oled_module;
#endif
```

If the user does not define `#define INTEGRALL_ENABLE_OLED` in their sketch, the compiler permanently removes the OLED code, saving RAM and Flash.

---

## 5. Memory Management Rules

To prevent heap fragmentation (the #1 cause of IoT crashes):
1. **No `String` Returns in the Core Loop:** Framework functions should return `bool` or primitive integers. Dynamic `String` objects are only allowed for high-level setup returns (like `httpGet()`) where they are instantly destroyed.
2. **Static JSON Buffers:** Telemetry generation uses `StaticJsonDocument<512>` on the stack, NEVER `DynamicJsonDocument` on the heap, preventing runaway fragmentation over days of up-time.

---

## 6. The "Easy HTTP" Subsystem

Networking is inherently blocking. When you use `integrall.httpGet(url)`, the flow is heavily guarded:
1. **Pre-Flight:** Checks `isOnline()`. If the WiFi router is dead, it instantly returns `""` rather than locking the processor for 10 seconds.
2. **Execution:** Allocates the `HTTPClient` locally.
3. **Destruction:** Ensures `.end()` is called regardless of timeout or success, freeing the TCP sockets.

---

## 7. Future Proofing: Event-Driven Upgrades

To push Integrall to v2.0, we will transition from polling to an **Event-Driven Architecture**.

Instead of checking states manually:
```cpp
if (integrall.motionDetected(5)) { ... }
```
Users will bind callbacks:
```cpp
integrall.events.on("motion", []() { 
    integrall.buzzerAlert(); 
});
```

This will decouple logic entirely from the main `loop()`, achieving true RTOS-like behavior while maintaining Arduino simplicity.
