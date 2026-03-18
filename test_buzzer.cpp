// ❌ THE OLD WAY (Blocking Audio & Ugly Math)
int buzzerPin = 8;

void setup() {
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  // Creating a "Success" double beep pattern freezes your code!
  tone(buzzerPin, 1500); // Start low tone
  delay(100);            // ⚠️ Blocking! Sensons can't read.
  noTone(buzzerPin);
  delay(50);             // ⚠️ Blocking! WiFi can disconnect.
  tone(buzzerPin, 2500); // High tone
  delay(100);            // ⚠️ Blocking!
  noTone(buzzerPin);

  delay(5000); 
}

// ==========================================

// ✅ THE INTEGRALL WAY (Non-Blocking Audio Patterns)
#define INTEGRALL_ENABLE_BUZZER
#include <Integrall.h>

Integrall::System integrall;

void setup() {
  integrall.begin();
  
  // 1. Initialize buzzer on pin 8
  integrall.enableBuzzer(8);
  
  // 2. Play a pre-built melodic success tone instantly in the background!
  integrall.buzzerSuccess(); 
}

void loop() {
  // 3. You can trigger custom, non-blocking audio patterns easily:
  if (/* Error Condition */ false) {
    // Beep 3 times, 150ms ON, 100ms OFF, at 2000Hz. Zero delay() calls!
    integrall.buzzerPattern(3, 150, 100, 2000); 
  }

  // The background engine calculates the tone timing and plays the melody! 🎵
  integrall.handle(); 
}
