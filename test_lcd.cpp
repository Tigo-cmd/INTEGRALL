// ❌ THE OLD WAY (Blocking & Complex)
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
String longText = "This string is way too long for a 20 character screen!";
int scrollPos = 0;
unsigned long lastScrollTime = 0;

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
}

void loop() {
  // Manual, math-heavy background scrolling
  if (millis() - lastScrollTime > 300) {
    lastScrollTime = millis();
    lcd.setCursor(0, 1);
    
    // String splitting logic to fake a scroll
    if (scrollPos < longText.length()) {
      lcd.print((longText + "   ").substring(scrollPos, scrollPos + 20));
      scrollPos++;
    } else {
      scrollPos = 0; // Reset
    }
  }
}

// ==========================================

// ✅ THE INTEGRALL WAY (Smart & Simple)
#define INTEGRALL_ENABLE_LCD
#include <Integrall.h>

Integrall::System integrall;

void setup() {
  integrall.enableLCD(0x27, 20, 4); // Handles Wire.begin() and init automatically
  
  integrall.lcdPrint("System Ready", 0, 0);
  
  // Integrall notices this is >20 chars and auto-scrolls it in the background!
  integrall.lcdPrint("This string is way too long for a 20 character screen!", 0, 1); 
}

void loop() {
  integrall.handle(); // Processes the scroll animation cleanly without blocking
}
