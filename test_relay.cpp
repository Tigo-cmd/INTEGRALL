// ❌ THE OLD WAY (Dangerous & Complex)
int pumpPin = 4;
int valvePin = 5;
unsigned long pumpStartTime = 0;
bool pumpRunning = false;
int buttonPin = 12;
bool lastButtonState = HIGH;

void setup() {
  pinMode(pumpPin, OUTPUT);
  pinMode(valvePin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  // Manual safety timeout (Easy to mess up and flood a room)
  if (pumpRunning && millis() - pumpStartTime > 10000) {
    digitalWrite(pumpPin, LOW); 
    pumpRunning = false;
  }
  
  // Manual button debouncing (Spaghetti code)
  bool buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState && buttonState == LOW) {
    delay(50); // Blocking debounce
    pumpRunning = !pumpRunning;
    digitalWrite(pumpPin, pumpRunning ? HIGH : LOW);
    if (pumpRunning) pumpStartTime = millis();
  }
  lastButtonState = buttonState;
}

// ==========================================

// ✅ THE INTEGRALL WAY (Industrial Safety, 0 Boilerplate)
#define INTEGRALL_ENABLE_RELAY
#include <Integrall.h>

Integrall::System integrall;

void setup() {
  integrall.begin();
  
  // Initialize Relay #0 on GPIO 4 (Active HIGH)
  integrall.relay.addRelay(0, 4, false);
  
  // 1. Safety Timeout: Auto-shutdown after 10 seconds perfectly
  integrall.relay.setSafetyTimeout(0, 10000);
  
  // 2. Physical UI: Attach button on GPIO 12 with auto-debouncing!
  integrall.relay.attachTrigger(0, 12, true);
}

void loop() {
  integrall.handle(); // The background engine keeps everything safe 🛡️
}
