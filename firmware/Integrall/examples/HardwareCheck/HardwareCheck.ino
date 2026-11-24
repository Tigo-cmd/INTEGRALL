/**
 * HardwareCheck.ino — guided bench test for real parts.
 *
 * Wire whatever you have (missing parts are reported, not fatal), open the
 * Serial Monitor at 115200, and follow the prompts. Each step prints PASS,
 * FAIL or SKIP. Send 's' to skip the current step. Runs on ESP32, ESP8266
 * and Uno/Nano — pins are chosen below per board.
 *
 * Parts: relay module, I2C LCD 16x2, buzzer, servo, HC-SR04, DHT11/22,
 *        4x4 keypad, PIR. Everything is optional.
 *
 * Uno/Nano: all parts at once don't fit in 32 KB / 2 KB, so the test runs in
 * two groups. Default = relay, LCD, buzzer, HC-SR04, DHT, PIR. Uncomment the
 * next line and re-upload to test the servo + keypad.
 */
// #define CHECK_GROUP_B
#if defined(__AVR__) && defined(CHECK_GROUP_B)
  #define INTEGRALL_ENABLE_SERVO
  #define INTEGRALL_ENABLE_KEYPAD
#elif defined(__AVR__)
  #define INTEGRALL_ENABLE_RELAY
  #define INTEGRALL_ENABLE_LCD
  #define INTEGRALL_ENABLE_BUZZER
  #define INTEGRALL_ENABLE_SENSORS
#else
  #define INTEGRALL_ENABLE_RELAY
  #define INTEGRALL_ENABLE_LCD
  #define INTEGRALL_ENABLE_BUZZER
  #define INTEGRALL_ENABLE_SERVO
  #define INTEGRALL_ENABLE_SENSORS
  #define INTEGRALL_ENABLE_KEYPAD
#endif
#include <Integrall.h>

// ---- pins --------------------------------------------------------------------
#if defined(ESP32)
const uint8_t RELAY = 26, BUZZER = 25, SERVO = 13, TRIG = 5, ECHO = 18, DHT_PIN = 4, PIR = 27;
byte rowPins[4] = {19, 23, 32, 33}, colPins[4] = {12, 14, 15, 2};
#elif defined(ESP8266)
// NodeMCU has few free pins: no keypad here (it would need 8).
const uint8_t RELAY = D5, BUZZER = D6, SERVO = D7, TRIG = D0, ECHO = D8, DHT_PIN = D4, PIR = D3;
#define NO_KEYPAD
#else   // Uno / Nano  (HC-SR04 echo on D2 = interrupt pin)
const uint8_t RELAY = 7, BUZZER = 8, SERVO = 9, TRIG = 4, ECHO = 2, DHT_PIN = 3, PIR = 6;
byte rowPins[4] = {A0, A1, A2, A3}, colPins[4] = {10, 11, 12, 13};
#endif
const uint8_t DHT_TYPE = 22;   // 11 for DHT11
// -------------------------------------------------------------------------------

Integrall::System integrall;
int relay = -1, servo = -1;
uint8_t step = 0, passed = 0, failed = 0, skipped = 0;
unsigned long stepStart = 0;
bool announced = false;

void result(const __FlashStringHelper* name, int r) {      // 1 pass, 0 fail, -1 skip
    integrall.print(r > 0 ? F("  [PASS] ") : (r == 0 ? F("  [FAIL] ") : F("  [SKIP] ")));
    Serial.println(name);
    if (r > 0) passed++; else if (r == 0) failed++; else skipped++;
    step++; announced = false; stepStart = millis();
}

void prompt(const __FlashStringHelper* text) {
    if (announced) return;
    announced = true;
    stepStart = millis();
    Serial.print(F("\n> "));
    Serial.print(text);
    Serial.println(F("  (y = it worked, n = it didn't, s = skip)"));
}

int answer() {                               // -2 = no answer yet
    if (!Serial.available()) return -2;
    char c = (char)Serial.read();
    if (c == 'y' || c == 'Y') return 1;
    if (c == 'n' || c == 'N') return 0;
    if (c == 's' || c == 'S') return -1;
    return -2;
}

void setup() {
    integrall.begin();
    integrall.println(F("\n=== Integrall hardware check ==="));
#if INTEGRALL_MODULE_LCD_ENABLED
    integrall.enableLCD();
#endif
    integrall.enableBuzzer(BUZZER);            // no-op when the buzzer module is off
#if INTEGRALL_MODULE_RELAY_ENABLED
    relay = integrall.enableRelay(RELAY, true, "test");
#endif
#if INTEGRALL_MODULE_SERVO_ENABLED
    servo = integrall.enableServo(SERVO, 90);
#endif
#if INTEGRALL_MODULE_KEYPAD_ENABLED && !defined(NO_KEYPAD)
    integrall.enableKeypad(rowPins, colPins);
#endif
    integrall.selfTest();                    // I2C scan + which modules answer
}

void loop() {
    integrall.handle();
    int a;
    switch (step) {
    case 0:  // LCD
#if !INTEGRALL_MODULE_LCD_ENABLED
        result(F("LCD (other test group)"), -1); break;
#else
        if (!integrall.lcdPresent()) { result(F("LCD (not found on I2C)"), -1); break; }
        integrall.lcdPrint("Integrall check", 0, 0);
        integrall.lcdPrint("LCD OK?", 0, 1);
        prompt(F("Does the LCD show 'Integrall check / LCD OK?'"));
        if ((a = answer()) != -2) result(F("LCD text"), a);
#endif
        break;
    case 1:  // relay
#if !INTEGRALL_MODULE_RELAY_ENABLED
        result(F("Relay (other test group)"), -1);
#else
        if (!announced) integrall.relayOn(relay);
        prompt(F("Did the relay CLICK and its LED turn on?"));
        if ((a = answer()) != -2) { integrall.relayOff(relay); result(F("Relay"), a); }
#endif
        break;
    case 2:  // buzzer
#if !INTEGRALL_MODULE_BUZZER_ENABLED
        result(F("Buzzer (other test group)"), -1);
        break;
#endif
        if (!announced) integrall.buzzerPattern(3, 150, 100);
        prompt(F("Did you hear 3 beeps?"));
        if ((a = answer()) != -2) result(F("Buzzer"), a);
        break;
    case 3:  // servo (non-blocking sweep)
#if !INTEGRALL_MODULE_SERVO_ENABLED
        result(F("Servo (other test group)"), -1);
#else
        if (!announced) integrall.sweepServo(servo, 8);
        prompt(F("Is the servo sweeping back and forth?"));
        if ((a = answer()) != -2) { integrall.setServo(servo, 90); result(F("Servo"), a); }
#endif
        break;
#if !INTEGRALL_MODULE_SENSORS_ENABLED
    case 4: case 5: case 7: result(F("Sensor (other test group)"), -1); break;
#else
    case 4: { // HC-SR04: automatic
        float d = integrall.readDistance(TRIG, ECHO);
        prompt(F("Hold your hand ~10-20 cm in front of the HC-SR04"));
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 500) {
            lastPrint = millis();
            char b[12];
            integrall.println(F("    distance: %s cm"), Integrall::valid(d) ? Integrall::Util::fmtFloat(b, sizeof b, d, 1) : "no echo");
        }
        if (Integrall::valid(d) && d > 5 && d < 30) { result(F("HC-SR04 sees your hand"), 1); break; }
        if ((a = answer()) != -2) result(F("HC-SR04"), a);
        else if (millis() - stepStart > 20000) result(F("HC-SR04 (no reading in 20 s)"), 0);
        break;
    }
    case 5: { // DHT: automatic
        prompt(F("Reading DHT sensor (takes ~2 s)..."));
        float t = integrall.readTemperature(DHT_PIN, DHT_TYPE), h = integrall.readHumidity(DHT_PIN, DHT_TYPE);
        if (Integrall::valid(t) && Integrall::valid(h) && t > -20 && t < 60 && h >= 0 && h <= 100) {
            char tb[10], hb[10];
            integrall.println(F("    %s C, %s %%"), Integrall::Util::fmtFloat(tb, sizeof tb, t), Integrall::Util::fmtFloat(hb, sizeof hb, h, 0));
            result(F("DHT temperature/humidity"), 1);
        } else if (millis() - stepStart > 6000) {
            result(F("DHT (no valid reading: check pin/type/pull-up)"), 0);
        } else if ((a = answer()) == -1) result(F("DHT"), -1);
        break;
    }
#endif
    case 6:  // keypad: automatic
#if defined(NO_KEYPAD) || !INTEGRALL_MODULE_KEYPAD_ENABLED
        result(F("Keypad (not in this test group / board)"), -1);
#else
    {
        prompt(F("Press 1, 2, 3 on the keypad"));
        static uint8_t seen = 0;
        char k = integrall.keypadGetKey();
        if (k) {
            integrall.println(F("    key '%c'"), k);
            if (k == "123"[seen]) seen++; else seen = 0;
        }
        if (seen == 3) { seen = 0; result(F("Keypad"), 1); }
        else if ((a = answer()) != -2) result(F("Keypad"), a);
        else if (millis() - stepStart > 30000) result(F("Keypad (no keys in 30 s)"), 0);
    }
#endif
        break;
#if INTEGRALL_MODULE_SENSORS_ENABLED
    case 7: { // PIR: automatic (PIR needs ~30-60 s warm-up after power-on)
        prompt(F("Wave in front of the PIR (it needs ~30 s warm-up after power-on)"));
        if (integrall.motionDetected(PIR)) { result(F("PIR motion"), 1); break; }
        if ((a = answer()) != -2) result(F("PIR"), a);
        else if (millis() - stepStart > 60000) result(F("PIR (no motion in 60 s)"), 0);
        break;
    }
#endif
    case 8:
        integrall.println(F("\n=== done: %u passed, %u failed, %u skipped ==="), (unsigned)passed, (unsigned)failed, (unsigned)skipped);
#if INTEGRALL_MODULE_LCD_ENABLED
        integrall.lcdPrint(failed ? "Check: FAILURES" : "Check: ALL OK", 0, 0);
        integrall.lcdPrint("", 0, 1);
#endif
        step++;
        break;
    default:
        break;
    }
}
