/**
 * Integrall.h
 * 
 * Main header for Integrall IoT Framework
 * 
 * This is the only file users need to include in their sketches.
 * All module configuration is done via #define flags BEFORE including this header.
 * 
 * Example usage:
 * ```cpp
 * #define INTEGRALL_ENABLE_RELAY
 * #define INTEGRALL_DEBUG_LEVEL 2
 * #include <Integrall.h>
 * 
 * Integrall::System integrall;
 * 
 * void setup() {
 *     Integrall::DeviceConfig config;
 *     config.wifi_ssid = "MyWiFi";
 *     config.wifi_password = "secret";
 *     config.backend_url = "http://192.168.1.100:8000";
 *     config.api_key = "my-api-key";
 *     
 *     integrall.begin(config);
 *     integrall.enableRelay(5);  // GPIO 5
 * }
 * 
 * void loop() {
 *     integrall.handle();
 * }
 * ```
 */

#ifndef INTEGRALL_H
#define INTEGRALL_H

/**
 * INTEGRALL ARCHITECTURE CHECK
 * This framework is designed for ESP32 and ESP8266, but supports 
 * standard Arduino boards in offline-only mode.
 */
#if !defined(ESP32) && !defined(ESP8266)
  #warning "Integrall: This board does not support networking. Switching to offline-only mode."
#endif

// ============================================================================
// USER CONFIGURATION SECTION
// Users define flags here or in their sketch BEFORE including this header
// ============================================================================

// Uncomment or define in sketch to enable modules:
// #define INTEGRALL_ENABLE_RELAY
// #define INTEGRALL_ENABLE_KEYPAD
// #define INTEGRALL_ENABLE_OLED
// #define INTEGRALL_ENABLE_LCD

// Debug level (0=none, 1=errors, 2=warnings, 3=info, 4=verbose)
// #define INTEGRALL_DEBUG_LEVEL 2

// ============================================================================
// FRAMEWORK INCLUDES
// ============================================================================

#include "config/IntegrallConfig.h"
#include "core/Logger.h"
#include "core/DeviceManager.h"

// Conditionally include modules based on user flags
#if INTEGRALL_MODULE_RELAY_ENABLED
#include "modules/RelayModule.h"
#endif

#if INTEGRALL_MODULE_LCD_ENABLED
#include "modules/LCDModule.h"
#endif

#if INTEGRALL_MODULE_SENSORS_ENABLED || INTEGRALL_MODULE_IMU_ENABLED
#include "modules/SensorModule.h"
#endif

#if INTEGRALL_MODULE_SERVO_ENABLED
#include "modules/ServoModule.h"
#endif

#if INTEGRALL_MODULE_KEYPAD_ENABLED
#include "modules/KeypadModule.h"
#endif

#if INTEGRALL_MODULE_OLED_ENABLED
#include "modules/OLEDModule.h"
#endif

#if INTEGRALL_MODULE_BUZZER_ENABLED
#include "modules/BuzzerModule.h"
#endif

#if INTEGRALL_MODULE_RGB_ENABLED
#include "modules/RGBModule.h"
#endif

#if INTEGRALL_MODULE_CAMERA_ENABLED
#include "modules/CameraModule.h"
#endif

#if INTEGRALL_MODULE_INPUT_ENABLED
#include "modules/InputModule.h"
#endif

#if INTEGRALL_MODULE_STORAGE_ENABLED
#include "modules/StorageModule.h"
#endif

#if INTEGRALL_MODULE_CELLULAR_ENABLED
#include "modules/CellularModule.h"
#endif

#if INTEGRALL_MODULE_TIME_ENABLED
#include "modules/TimeModule.h"
#endif

#if INTEGRALL_MODULE_COMM_ENABLED
#include "modules/CommunicationModule.h"
#endif

#if INTEGRALL_MODULE_POWER_ENABLED
#include "modules/PowerModule.h"
#endif

#if INTEGRALL_MODULE_AUDIO_ENABLED
#include "modules/AudioModule.h"
#endif

#if INTEGRALL_MODULE_STEPPER_ENABLED
#include "modules/StepperModule.h"
#endif

#include "modules/BlinkerModule.h"
#include "modules/WebModule.h"

// ============================================================================
// MAIN SYSTEM CLASS
// ============================================================================

namespace Integrall {

class System {
public:
    System();
    ~System();
    
    /**
     * Initialize the Integrall system with a config struct
     */
    bool begin(const DeviceConfig& config);
    
    /**
     * Standard WiFi Station Mode - Connects to your router
     */
    bool begin(const char* ssid, const char* pass, const char* url = nullptr) {
        DeviceConfig config;
        config.wifi_ssid = ssid;
        config.wifi_password = pass;
        config.backend_url = url;
        config.use_wifi_manager = false;
        return begin(config);
    }

    /**
     * Access Point (AP) Mode - ESP32 creates its own network
     */
    bool beginAP(const char* ap_ssid, const char* ap_password = nullptr) {
        DeviceConfig config;
        config.wifi_ssid = ap_ssid;
        config.wifi_password = ap_password;
        config.use_wifi_manager = true; // Use portal/AP logic
        return begin(config);
    }
    
    /**
     * Start the system in Offline Mode (No WiFi enabled)
     */
    bool begin();
    
    /**
     * Main loop handler - call this in your loop() function
     */
    void handle();
    
    /**
     * Simple formatted printing to the Serial monitor (acts like printf).
     * @param format The string format (e.g. "Angle: %d degrees")
     */
    void print(const char* format, ...) {
        #if INTEGRALL_DEBUG_LEVEL > 0
        if (Logger::ready()) {
            char buffer[128];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            Serial.print(buffer);
        }
        #else
        (void)format;
        #endif
    }

    /**
     * Simple formatted printing to the Serial monitor with a newline.
     * @param format The string format (e.g. "Angle: %d degrees")
     */
    void println(const char* format, ...) {
        #if INTEGRALL_DEBUG_LEVEL > 0
        if (Logger::ready()) {
            char buffer[128];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            Serial.println(buffer);
        }
        #else
        (void)format;
        #endif
    }

    
    // ========================================================================
    // RELAY MODULE API (available if INTEGRALL_ENABLE_RELAY is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_RELAY_ENABLED
    
    /**
     * Enable relay control on specified pin
     * @param pin GPIO pin number
     * @param active_low true if relay triggers on LOW (default: true for optocoupled modules)
     * @param name Optional name for identification
     * @return relay index for subsequent operations
     */
    int enableRelay(uint8_t pin, bool active_low = true, const char* name = nullptr);
    
    /**
     * Turn relay on
     * @param relay_index Index returned by enableRelay()
     */
    void relayOn(int relay_index);
    
    /**
     * Turn relay off
     * @param relay_index Index returned by enableRelay()
     */
    void relayOff(int relay_index);
    
    /**
     * Toggle relay state
     * @param relay_index Index returned by enableRelay()
     */
    void relayToggle(int relay_index);

    /**
     * Set relay state directly
     * @param relay_index The index returned by enableRelay
     * @param on          True for ON, False for OFF
     * @param alert       If true, plays the alert pattern when ON
     */
    void setRelay(int relay_index, bool on, bool alert = false) {
        if (on) {
            relayOn(relay_index);
            #if INTEGRALL_MODULE_BUZZER_ENABLED
            if (alert) buzzerAlert();
            #endif
        } else {
            relayOff(relay_index);
        }
    }
    
    /**
     * Set relay safety timeout (auto-off after duration)
     * @param relay_index Index returned by enableRelay()
     * @param duration_ms Timeout in milliseconds (0 to disable)
     */
    void relaySetTimeout(int relay_index, uint32_t duration_ms);
    
    /**
     * Assign relay to interlock group (prevents simultaneous activation)
     * @param relay_index Index returned by enableRelay()
     * @param group Group number (0 = no interlock)
     */
    void relaySetInterlock(int relay_index, uint8_t group);
    
    /**
     * Attach physical button to control relay
     * @param relay_index Index returned by enableRelay()
     * @param button_pin GPIO pin for button (uses internal pullup)
     * @param active_low true if button connects to ground when pressed
     */
    void relayAttachButton(int relay_index, uint8_t button_pin, bool active_low = true);
    
    /**
     * Get relay state
     * @param relay_index Index returned by enableRelay()
     * @return true if relay is on
     */
    bool relayIsOn(int relay_index);
    
    /**
     * Turn all relays off / on and inspect statistics
     */
    void allRelaysOff();
    void allRelaysOn() { _relay_module.allOn(); }
    uint32_t relayGetOnTime(int relay_index) const { return (relay_index >= 0 && relay_index < _relay_count) ? _relay_module.getTotalOnTime(relay_index) : 0; }
    uint8_t  relayGetToggleCount(int relay_index) const { return (relay_index >= 0 && relay_index < _relay_count) ? _relay_module.getToggleCount(relay_index) : 0; }
    
    #endif  // INTEGRALL_MODULE_RELAY_ENABLED
    
    // ========================================================================
    // LCD MODULE API (available if INTEGRALL_ENABLE_LCD is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_LCD_ENABLED
    /**
     * Initialize the LCD manually with custom dimensions
     */
    bool enableLCD(uint8_t address = 0x27, uint8_t cols = 16, uint8_t rows = 2) {
        return _lcd_module.begin(address, cols, rows);
    }

    /**
     * Print text to I2C LCD
     * @param text Text to display
     * @param col Column (0-15)
     * @param row Row (0-1)
     */
    void lcdPrint(const char* text, uint8_t col = 0, uint8_t row = 0) {
        _lcd_module.print(text, col, row);
    }
    
    /**
     * Clear the LCD screen
     */
    void lcdClear() {
        _lcd_module.clear();
    }
    
    /**
     * Set the LCD backlight on or off
     */
    void lcdBacklight(bool on) {
        _lcd_module.setBacklight(on);
    }

    /**
     * Professional Non-blocking Scroll
     * Best used for long text in the loop().
     */
    void lcdScrollText(const char* text, uint8_t row = 0, uint16_t speed_ms = 300) {
        _lcd_module.scrollText(text, row, speed_ms);
    }

    /**
     * Control the LCD blinking/fixed cursor
     */
    void lcdCursor(bool visible, bool blink = false) {
        _lcd_module.setCursorVisible(visible, blink);
    }

    /**
     * Load a custom shape into memory (0-7 slots)
     */
    void lcdCreateChar(uint8_t slot, uint8_t charMap[]) {
        _lcd_module.createCustomChar(slot, charMap);
    }
    
    #endif  // INTEGRALL_MODULE_LCD_ENABLED
    
    // ========================================================================
    // SENSOR MODULE API (available if INTEGRALL_ENABLE_SENSORS is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_SENSORS_ENABLED
    
    /**
     * Read distance in CM with noise filtering
     * @param trig Trigger pin
     * @param echo Echo pin
     * @param samples Number of readings to average (default: 3)
     */
    float readDistance(uint8_t trig, uint8_t echo, uint8_t samples = 3) {
        return _sensor_module.readDistanceCM(trig, echo, samples);
    }

    float readDistanceCM(uint8_t trig, uint8_t echo, uint8_t samples = 3) {
        return _sensor_module.readDistanceCM(trig, echo, samples);
    }

    /**
     * Easy proximity alert / range check
     */
    bool isNear(uint8_t trig, uint8_t echo, float threshold_cm) {
        return _sensor_module.isWithinRange(trig, echo, 0, threshold_cm);
    }

    bool isWithinRange(uint8_t trig, uint8_t echo, float min_cm, float max_cm) {
        return _sensor_module.isWithinRange(trig, echo, min_cm, max_cm);
    }

    /**
     * Read analog input as a percentage (0-100) or voltage
     */
    int readAnalogPercent(uint8_t pin) {
        return _sensor_module.readAnalogPercent(pin);
    }

    int readPercent(uint8_t pin, bool inverted = false) {
        return _sensor_module.readPercent(pin, inverted);
    }

    float readPinVoltage(uint8_t pin, uint8_t samples = 5) {
        return _sensor_module.readPinVoltage(pin, samples);
    }

    /**
     * Read LDR light intensity as a percentage (0-100)
     * @param reverse Set to true if 0=Bright (common with pull-up LDRs)
     */
    int readLightPercent(uint8_t pin, bool reverse = false) {
        return _sensor_module.readLightPercent(pin, reverse);
    }

    /**
     * Digital & Security Sensor Triggers (PIR, Flame, Tilt, Reed Door Switch, Vibration, Sound, Hall, Touch)
     */
    bool isTriggered(uint8_t pin, bool activeHigh = true) {
        return _sensor_module.isTriggered(pin, activeHigh);
    }

    bool motionDetected(uint8_t pin)                        { return _sensor_module.motionDetected(pin); }
    bool flameDetected(uint8_t pin, bool activeLow = true)  { return _sensor_module.flameDetected(pin, activeLow); }
    bool tiltDetected(uint8_t pin, bool activeLow = true)   { return _sensor_module.tiltDetected(pin, activeLow); }
    bool vibrationDetected(uint8_t pin, bool activeHigh = true) { return _sensor_module.vibrationDetected(pin, activeHigh); }
    bool soundDetected(uint8_t pin, bool activeLow = true)  { return _sensor_module.soundDetected(pin, activeLow); }
    bool magneticDetected(uint8_t pin, bool activeLow = true) { return _sensor_module.magneticDetected(pin, activeLow); }
    bool touchDetected(uint8_t pin, bool activeHigh = true) { return _sensor_module.touchDetected(pin, activeHigh); }
    bool doorOpen(uint8_t pin, bool usePullup = true)       { return _sensor_module.doorOpen(pin, usePullup); }

    /**
     * Soil Moisture & Rain / Water Level Sensors
     */
    int  readSoilMoisture(uint8_t pin, int dryRaw = -1, int wetRaw = -1) { return _sensor_module.readSoilMoisture(pin, dryRaw, wetRaw); }
    int  readRainPercent(uint8_t pin, bool activeLow = true)             { return _sensor_module.readRainPercent(pin, activeLow); }
    bool isRaining(uint8_t pin, int thresholdPct = 30, bool activeLow = true) { return _sensor_module.isRaining(pin, thresholdPct, activeLow); }

    /**
     * Temperature & Environmental Sensors (DS18B20, BME280, BMP280, DHT11/22, SHT3x, NTC Thermistor, LM35, TMP36)
     */
    float readProbeTemp(uint8_t pin, bool celsius = true) {
        return _sensor_module.readProbeTemp(pin, celsius);
    }

    void readEnvironment(uint8_t addr, float &temp, float &hum, float &pres) {
        _sensor_module.readEnvironment(addr, temp, hum, pres);
    }

    float readPressure(uint8_t addr = 0x76) {
        return _sensor_module.readPressure(addr);
    }

    float readTemperature(uint8_t pin, uint8_t type = 11, bool celsius = true) {
        return _sensor_module.readTemperature(pin, type, celsius);
    }

    float readHumidity(uint8_t pin, uint8_t type = 11) {
        return _sensor_module.readHumidity(pin, type);
    }

    bool readSHT3x(uint8_t addr, float &tempC, float &humidity) {
        return _sensor_module.readSHT3x(addr, tempC, humidity);
    }

    float readThermistorC(uint8_t pin, float beta = 3950.0f, float r0 = 10000.0f, float seriesR = 10000.0f, bool ntcToGnd = true) {
        return _sensor_module.readThermistorC(pin, beta, r0, seriesR, ntcToGnd);
    }

    float readLM35C(uint8_t pin)  { return _sensor_module.readLM35C(pin); }
    float readTMP36C(uint8_t pin) { return _sensor_module.readTMP36C(pin); }

    /**
     * Light, UV, Color & Optical Distance Sensors (BH1750, GUVA-S12SD, TCS3200, Sharp IR, VL53L0X ToF)
     */
    float readLux(uint8_t addr = 0x23) { return _sensor_module.readLux(addr); }
    float readUVIndex(uint8_t pin)     { return _sensor_module.readUVIndex(pin); }
    void  readColorRGB(uint8_t s2Pin, uint8_t s3Pin, uint8_t outPin, uint8_t &r, uint8_t &g, uint8_t &b) {
        _sensor_module.readColorRGB(s2Pin, s3Pin, outPin, r, g, b);
    }
    float readIRDistanceCM(uint8_t pin) { return _sensor_module.readIRDistanceCM(pin); }
    int   readLaserDistanceMM()         { return _sensor_module.readLaserDistanceMM(); }

    /**
     * Air Quality, Gas, Smoke, PM2.5 Dust & CO2 Sensors (MQ-2/4/7/135, GP2Y1010AU0F, MH-Z19B)
     */
    float readGasPPM(uint8_t pin, float r0 = 10.0f, float rl = 10.0f, float a = 116.602f, float b = -2.769f) {
        return _sensor_module.readGasPPM(pin, r0, rl, a, b);
    }
    float readDustDensity(uint8_t ledPin, uint8_t analogPin) {
        return _sensor_module.readDustDensity(ledPin, analogPin);
    }
    int readCO2PWM(uint8_t pwmPin, int rangePPM = 5000) {
        return _sensor_module.readCO2PWM(pwmPin, rangePPM);
    }

    /**
     * Water Quality & Liquid Flow Sensors (TDS, pH, Turbidity NTU, YF-S201 Flow Rate)
     */
    float readTDS(uint8_t pin, float waterTempC = 25.0f) { return _sensor_module.readTDS(pin, waterTempC); }
    float readPH(uint8_t pin, float neutralVoltage = 2.50f, float slope = -5.70f) { return _sensor_module.readPH(pin, neutralVoltage, slope); }
    float readTurbidityNTU(uint8_t pin) { return _sensor_module.readTurbidityNTU(pin); }
    float readWaterFlowLPM(uint8_t pin, float calibrationFactor = 7.5f) { return _sensor_module.readWaterFlowLPM(pin, calibrationFactor); }

    /**
     * Load Cell Weight (HX711), Current (ACS712 AC/DC), Voltage Divider & Heart-Rate Pulse Sensors
     */
    long  readWeightRaw(uint8_t doutPin, uint8_t sckPin, uint32_t timeoutMs = 200) { return _sensor_module.readWeightRaw(doutPin, sckPin, timeoutMs); }
    float readWeightGrams(uint8_t doutPin, uint8_t sckPin, long tareOffset = 0, float scaleFactor = 420.0f) { return _sensor_module.readWeightGrams(doutPin, sckPin, tareOffset, scaleFactor); }
    float readCurrentDC(uint8_t pin, float sensitivityMvPerA = 185.0f, float zeroPointV = -1.0f) { return _sensor_module.readCurrentDC(pin, sensitivityMvPerA, zeroPointV); }
    float readCurrentAC(uint8_t pin, float sensitivityMvPerA = 185.0f, uint16_t windowMs = 20) { return _sensor_module.readCurrentAC(pin, sensitivityMvPerA, windowMs); }
    float readVoltageDivider(uint8_t pin, float dividerRatio = 5.0f) { return _sensor_module.readVoltageDivider(pin, dividerRatio); }
    int   readHeartPulsePercent(uint8_t pin) { return _sensor_module.readHeartPulsePercent(pin); }

    /**
     * Atmosphere Math helpers
     */
    float calculateAltitude(float pressure, float seaLevel = 1013.25) { return _sensor_module.calculateAltitude(pressure, seaLevel); }
    float calculateDewPoint(float temp, float humidity) { return _sensor_module.calculateDewPoint(temp, humidity); }
    float calculateHeatIndex(float temp, float humidity) { return _sensor_module.calculateHeatIndex(temp, humidity); }
    
    #endif // INTEGRALL_MODULE_SENSORS_ENABLED

    // ========================================================================
    // IMU MODULE API (available if INTEGRALL_ENABLE_IMU is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_IMU_ENABLED
    
    /**
     * Start the MPU6050 IMU sensor
     */
    bool enableIMU(uint8_t sda = 21, uint8_t scl = 22) {
        return _sensor_module.beginIMU(sda, scl);
    }
    
    /**
     * Get total acceleration magnitude (Impact Force)
     */
    float getImpactForce() {
        return _sensor_module.getImpactForce();
    }
    
    /**
     * Get the posture state string ("Standing", "Sitting", "Fallen")
     */
    const char* getPosture() {
        return _sensor_module.getPostureString();
    }
    
    /**
     * Determine if the device is currently horizontal
     */
    bool isHorizontal() {
        return _sensor_module.isHorizontal();
    }

    float getPitch() { return _sensor_module.getPitch(); }
    float getRoll()  { return _sensor_module.getRoll(); }
    float getIMUTemperature() { return _sensor_module.getIMUTemperature(); }

    #endif // INTEGRALL_MODULE_IMU_ENABLED

    // ========================================================================
    // CELLULAR MODULE API (available if INTEGRALL_ENABLE_CELLULAR is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_CELLULAR_ENABLED
    
    /**
     * Start the SIM800L module
     */
    bool cellularBegin(uint8_t rxPin = 16, uint8_t txPin = 17) {
        return _cellular_module.begin(rxPin, txPin);
    }
    
    /**
     * Send an emergency SMS
     */
    void sendSMS(const char* number, const char* message) {
        _cellular_module.sendSMS(number, message);
    }
    
    #endif // INTEGRALL_MODULE_CELLULAR_ENABLED

    // ========================================================================
    // SERVO MODULE API (available if INTEGRALL_ENABLE_SERVO is defined)
    // ========================================================================
    // (Legacy Servo API removed - see new multi-servo API below)

    // ========================================================================
    // KEYPAD MODULE API (available if INTEGRALL_ENABLE_KEYPAD is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_KEYPAD_ENABLED

    /**
     * Initialize keypad with row and column GPIO pins
     * @param rowPins Array of row pins
     * @param colPins Array of column pins
     * @param rows    Number of rows (default 4)
     * @param cols    Number of columns (default 4)
     */
    bool enableKeypad(byte* rowPins, byte* colPins, byte rows = 4, byte cols = 4) {
        return _keypad_module.begin(rowPins, colPins, rows, cols);
    }

    /**
     * Get the key currently pressed ('\0' if none)
     */
    char keypadGetKey() {
        return _keypad_module.getKey();
    }

    /**
     * Get the last key that was pressed
     */
    char keypadLastKey() const {
        return _keypad_module.getLastKey();
    }

    /**
     * Accumulate keypresses into a string buffer.
     * '#' submits, '*' is backspace.
     */
    const char* keypadCapture(uint8_t maxLen = 8) {
        return _keypad_module.captureString(maxLen);
    }

    /**
     * Returns true when '#' is pressed and buffer has content
     */
    bool keypadSubmitted() {
        return _keypad_module.isSubmitted();
    }

    /**
     * Compare buffered input to a PIN/password
     */
    bool keypadCheckPin(const char* pin) {
        bool ok = _keypad_module.checkPin(pin);
        _keypad_module.clearBuffer();
        return ok;
    }

    /**
     * Clear the keypad input buffer
     */
    void keypadClear() {
        _keypad_module.clearBuffer();
    }

    #endif // INTEGRALL_MODULE_KEYPAD_ENABLED

    // ========================================================================
    // HIGH-LEVEL LOCK SYSTEM
    // Requires INTEGRALL_ENABLE_KEYPAD + INTEGRALL_ENABLE_RELAY
    // Optionally uses INTEGRALL_ENABLE_LCD for feedback
    // ========================================================================
    #if INTEGRALL_MODULE_KEYPAD_ENABLED && INTEGRALL_MODULE_RELAY_ENABLED

    /**
     * Configure the lock system in one call.
     * @param pin         The secret PIN/password to check against
     * @param relayIndex  The relay index returned by enableRelay()
     * @param unlockMs    How long to keep the relay on (ms), default 5s
     * @param maxRetries  Lock out after N wrong attempts (0 = unlimited)
     */
    void lockSetup(const char* pin, int relayIndex,
                   uint32_t unlockMs = 5000, uint8_t maxRetries = 3) {
        if (pin) {
            INTEGRALL_SAFE_STRCPY(_lock_pin, pin, sizeof(_lock_pin));
        }
        _lock_relay      = relayIndex;
        _lock_unlock_ms  = unlockMs;
        _lock_max_tries  = maxRetries;
        _lock_tries      = 0;
        _lock_locked_out = false;
        _lock_state      = 0;
        _lock_state_ts   = 0;
        _lock_active     = true;
        relaySetTimeout(relayIndex, unlockMs);
        _lockShowIdle();
    }

    /**
     * Call this in loop() to run the entire lock system (non-blocking).
     * Handles: key capture, asterisk display, PIN check,
     *          LCD feedback, relay control, and lockout logic.
     */
    void lockUpdate() {
        if (!_lock_active || _lock_locked_out) return;

        unsigned long now = millis();
        if (_lock_state == 1) { // Unlocked display wait
            if (now - _lock_state_ts >= _lock_unlock_ms) {
                _lock_state = 0;
                _keypad_module.clearBuffer();
                _lockShowIdle();
            }
            return;
        }
        if (_lock_state == 2) { // Denied display wait
            if (now - _lock_state_ts >= 2000) {
                _lock_state = 0;
                _keypad_module.clearBuffer();
                _lockShowIdle();
            }
            return;
        }

        // Capture keypresses and show asterisk feedback on LCD
        const char* input = _keypad_module.captureString(8);

        #if INTEGRALL_MODULE_LCD_ENABLED
        // Show asterisks on row 1, padded with spaces, or "Enter PIN:" if empty
        char row1[17];
        uint8_t len = strlen(input);
        if (len == 0) {
            memcpy(row1, "  Enter PIN:    ", 17);
        } else {
            for (uint8_t i = 0; i < 16; i++)
                row1[i] = (i < len) ? '*' : ' ';
            row1[16] = '\0';
        }
        _lcd_module.print(row1, 0, 1);
        #endif

        // When '#' is pressed, evaluate the PIN
        if (_keypad_module.isSubmitted()) {
            if (_keypad_module.checkPin(_lock_pin)) {
                // Correct PIN
                _lock_tries = 0;
                #if INTEGRALL_MODULE_LCD_ENABLED
                _lcd_module.print("  ** UNLOCKED **", 0, 0);
                _lcd_module.print("   Welcome!     ", 0, 1);
                #endif
                #if INTEGRALL_MODULE_BUZZER_ENABLED
                _buzzer_module.success();
                #endif
                _logEvent("lock", "unlocked");
                relayOn(_lock_relay);
                _lock_state = 1;
                _lock_state_ts = now;
            } else {
                // Wrong PIN
                _lock_tries++;
                #if INTEGRALL_MODULE_BUZZER_ENABLED
                _buzzer_module.failure();
                #endif
                _logEvent("lock", "denied");
                if (_lock_max_tries > 0 && _lock_tries >= _lock_max_tries) {
                    // Lockout activated
                    _lock_locked_out = true;
                    #if INTEGRALL_MODULE_LCD_ENABLED
                    _lcd_module.print(" !! LOCKED OUT !", 0, 0);
                    _lcd_module.print("  Call Admin    ", 0, 1);
                    #endif
                } else {
                    #if INTEGRALL_MODULE_LCD_ENABLED
                    char msg[24];
                    snprintf(msg, sizeof(msg), " Wrong! (%u left)",
                             (unsigned int)(_lock_max_tries - _lock_tries));
                    _lcd_module.print("  !! DENIED !!  ", 0, 0);
                    _lcd_module.print(msg, 0, 1);
                    #endif
                    _lock_state = 2;
                    _lock_state_ts = now;
                }
            }
            _keypad_module.clearBuffer();
        }
    }

    /**
     * Reset a lockout (e.g., called from a master PIN or admin button)
     */
    void lockReset() {
        _lock_tries      = 0;
        _lock_locked_out = false;
        _lock_state      = 0;
        _keypad_module.clearBuffer();
        _lockShowIdle();
    }

    #endif // INTEGRALL_MODULE_KEYPAD_ENABLED && INTEGRALL_MODULE_RELAY_ENABLED

    // ========================================================================
    // OLED MODULE API (available if INTEGRALL_ENABLE_OLED is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_OLED_ENABLED

    /**
     * Print text to the OLED screen
     * @param text  Text to display
     * @param col   Character column (0-indexed)
     * @param row   Character row (0-indexed)
     * @param clear Clear screen before printing
     */
    void oledPrint(const char* text, uint8_t col = 0, uint8_t row = 0, bool clear = false) {
        _oled_module.print(text, col, row, clear);
    }

    /**
     * Print a labeled value (e.g., "Temp: 24.5")
     */
    void oledPrintValue(const char* label, float value, uint8_t row = 0) {
        _oled_module.printValue(label, value, row);
    }

    /**
     * Draw a progress bar (0-100%)
     */
    void oledBar(uint8_t percent) {
        _oled_module.drawBar(percent);
    }

    /**
     * Clear the OLED screen
     */
    void oledClear() {
        _oled_module.clear();
    }

    /**
     * Get direct access to the Adafruit OLED Display object for advanced graphics
     */
    Adafruit_SSD1306* oled() {
        return _oled_module.getRaw();
    }

    #endif // INTEGRALL_MODULE_OLED_ENABLED

    // ========================================================================
    // BUZZER MODULE API (available if INTEGRALL_ENABLE_BUZZER is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_BUZZER_ENABLED

    bool enableBuzzer(uint8_t pin) { return _buzzer_module.begin(pin); }
    void buzzerBeep(uint32_t ms = 200)                          { _buzzer_module.beep(ms); }
    void buzzerTone(uint16_t freq, uint32_t ms = 200)           { _buzzer_module.customTone(freq, ms); }
    void buzzerPattern(uint8_t n, uint32_t on = 150, uint32_t off = 100) { _buzzer_module.pattern(n, on, off); }
    void buzzerAlert()   { _buzzer_module.alert(); }
    void buzzerSuccess() { _buzzer_module.success(); }
    void buzzerFail()    { _buzzer_module.failure(); }
    void buzzerOff()     { _buzzer_module.off(); }
    #else
    void buzzerSuccess() {}
    void buzzerFail()    {}
    void buzzerOff()     {}
    bool enableBuzzer(uint8_t pin) { (void)pin; return false; }
    void buzzerBeep(uint32_t ms = 200) { (void)ms; }
    void buzzerTone(uint16_t freq, uint32_t ms = 200) { (void)freq; (void)ms; }
    void buzzerPattern(uint8_t n, uint32_t on = 150, uint32_t off = 100) { (void)n; (void)on; (void)off; }
    void buzzerAlert()   {}
    #endif

    // ========================================================================
    // RGB LED API (available if INTEGRALL_ENABLE_RGB is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_RGB_ENABLED

    bool enableRGB(uint8_t r, uint8_t g, uint8_t b, bool commonAnode = false) {
        return _rgb_module.begin(r, g, b, commonAnode);
    }
    void setRGB(uint8_t r, uint8_t g, uint8_t b) { _rgb_module.set(r, g, b); }
    void setColor(const char* name)               { _rgb_module.setColor(name); }
    void rgbOff()                                 { _rgb_module.off(); }
    void rgbBlink(uint8_t r, uint8_t g, uint8_t b, uint32_t ms = 500) {
        _rgb_module.updateBlink(r, g, b, ms);
    }

    #endif // INTEGRALL_MODULE_RGB_ENABLED

    // ========================================================================
    // SERVO MODULE API (available if INTEGRALL_ENABLE_SERVO is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_SERVO_ENABLED

    /**
     * Initialize a servo motor on a specific pin, with an optional starting angle
     * @return The servo index ID (0 to 3) for multi-servo control, or -1 if full.
     */
    int enableServo(uint8_t pin, int startAngle = -1) {
        return _servo_module.attach(pin, startAngle);
    }

    /**
     * Move the servo to a specific angle (0 to 180) instantly
     */
    void setServo(uint8_t angle) { _servo_module.set(0, angle); }
    void setServo(int index, uint8_t angle) { _servo_module.set(index, angle); }

    /**
     * Map a joystick or potentiometer (analog pin) directly to the servo angle
     */
    void setServoFromAnalog(uint8_t analogPin) { _servo_module.setFromAnalog(0, analogPin); }
    void setServoFromAnalog(int index, uint8_t analogPin) { _servo_module.setFromAnalog(index, analogPin); }

    /**
     * Non-blocking sweep.
     * Single-servo: sweepServo(speed_ms) sweeps servo 0.
     * Multi-servo:  sweepServo(index, speed_ms) sweeps the specified servo.
     */
    void sweepServo(uint32_t speed_ms = 15) { _servo_module.sweep(0, speed_ms); }
    void sweepServo(int index, uint32_t speed_ms) { _servo_module.sweep(index, speed_ms); }

    /**
     * Non-blocking ease move. Slowly moves the servo to a target angle in the background.
     * Single-servo: easeServo(targetAngle, speed_ms) eases servo 0.
     * Multi-servo:  easeServo(index, targetAngle, speed_ms) eases the specified servo.
     */
    void easeServo(uint8_t targetAngle, uint32_t speed_ms = 15) { _servo_module.easeTo(0, targetAngle, speed_ms); }
    void easeServo(int index, uint8_t targetAngle, uint32_t speed_ms) { _servo_module.easeTo(index, targetAngle, speed_ms); }

    #endif // INTEGRALL_MODULE_SERVO_ENABLED

    // ========================================================================
    // INPUT MODULE API (available if INTEGRALL_ENABLE_INPUT is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_INPUT_ENABLED
    bool inputButtonPressed(uint8_t pin, bool activeLow = true) { return _input_module.buttonPressed(pin, activeLow); }
    int  inputReadJoystick(uint8_t pin, int center = 512, int deadzone = 50) { return _input_module.readJoystickAxis(pin, center, deadzone); }
    bool inputTouchActive(uint8_t pin) { return _input_module.touchActive(pin); }
    int  inputReadEncoder(uint8_t clk, uint8_t dt) { return _input_module.readEncoder(clk, dt); }
    #endif

    // ========================================================================
    // STORAGE MODULE API (available if INTEGRALL_ENABLE_STORAGE is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_STORAGE_ENABLED
    void storageBeginEEPROM(uint16_t size = 512) { _storage_module.beginEEPROM(size); }
    void storageWriteInt(int addr, int data)     { _storage_module.writeInt(addr, data); }
    int  storageReadInt(int addr)                { return _storage_module.readInt(addr); }
    #if INTEGRALL_SD_AVAILABLE
    bool storageLog(const char* path, const char* msg, uint8_t cs = 5) { return _storage_module.logToFile(path, msg, cs); }
    bool storageDelete(const char* path)                              { return _storage_module.deleteFile(path); }
    #endif
    #endif

    // ========================================================================
    // TIME MODULE API (available if INTEGRALL_ENABLE_TIME is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_TIME_ENABLED
    #if INTEGRALL_RTC_AVAILABLE
    bool   timeBeginRTC() { return _time_module.beginRTC(); }
    String timeGetStr()   { return _time_module.getTimeString(); }
    String timeGetISO()   { return _time_module.getISODateTime(); }
    #endif
    #if defined(ESP32) || defined(ESP8266)
    void timeSetupNTP(const char* srv = "pool.ntp.org", long gmt = 0, int dst = 0) { _time_module.setupNTP(srv, gmt, dst); }
    bool timeIsSynced() { return _time_module.isSyncDone(); }
    #endif
    #endif

    // ========================================================================
    // COMMUNICATION MODULE API (available if INTEGRALL_ENABLE_COMM is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_COMM_ENABLED
    #if INTEGRALL_BLE_AVAILABLE
    bool commBeginBLE(const char* n) { return _comm_module.beginBLE(n); }
    void commUpdateBLE(int v)        { _comm_module.updateBLEValue(v); }
    #endif
    #if INTEGRALL_LORA_AVAILABLE
    bool commBeginLoRa(long f, int ss = 18, int rst = 14, int dio0 = 26) { return _comm_module.beginLoRa(f, ss, rst, dio0); }
    void commPushLoRa(const char* p)                                     { _comm_module.pushLoRa(p); }
    #endif
    #endif

    // ========================================================================
    // POWER MODULE API (available if INTEGRALL_ENABLE_POWER is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_POWER_ENABLED
    #if INTEGRALL_INA219_AVAILABLE
    bool  powerBeginINA(uint8_t a = 0x40) { return _power_module.beginINA(a); }
    float powerGetVoltage()               { return _power_module.getVoltage(); }
    float powerGetCurrent()               { return _power_module.getCurrent(); }
    float powerGetPower()                 { return _power_module.getPower(); }
    #endif
    int  powerGetBattery(float v)         { return _power_module.getBatteryPercent(v); }
    void powerDeepSleep(uint32_t s)       { _power_module.deepSleep(s); }
    #endif

    // ========================================================================
    // AUDIO MODULE API (available if INTEGRALL_ENABLE_AUDIO is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_AUDIO_ENABLED
    #if INTEGRALL_DFPLAYER_AVAILABLE
    bool audioBeginDF(HardwareSerial& p, int rx = -1, int tx = -1) { return _audio_module.beginDFPlayer(p, rx, tx); }
    void audioPlay(int track)                                      { _audio_module.playTrack(track); }
    void audioVolume(int v)                                        { _audio_module.setVolume(v); }
    #endif
    int audioSoundLevel(uint8_t pin) { return _audio_module.readSoundLevel(pin); }
    #endif

    // ========================================================================
    // STEPPER MODULE API (available if INTEGRALL_ENABLE_STEPPER is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_STEPPER_ENABLED
    void stepperBegin(int spr, int p1, int p2, int p3, int p4) { _stepper_module.begin(spr, p1, p2, p3, p4); }
    void stepperSpeed(int rpm)                                { _stepper_module.setSpeed(rpm); }
    void stepperStep(int s)                                   { _stepper_module.step(s); }
    void stepperMove(int deg)                                 { _stepper_module.moveDegrees(deg); }
    #endif

    // extra input methods
    #if INTEGRALL_MODULE_INPUT_ENABLED
    #if INTEGRALL_RFID_AVAILABLE
    String inputReadRFID(uint8_t ss = 5, uint8_t rst = 0) { return _input_module.readRFID(ss, rst); }
    #endif
    #if INTEGRALL_IR_AVAILABLE
    uint32_t inputReadIR(uint8_t pin) { return _input_module.readIR(pin); }
    #endif
    #endif

    // extra time methods
    #if INTEGRALL_MODULE_TIME_ENABLED
    #if INTEGRALL_GPS_AVAILABLE
    void timeGetGPS(HardwareSerial& p, float &lat, float &lng) { _time_module.getGPSLocation(p, lat, lng); }
    #endif
    #endif

    // ========================================================================
    // CAMERA MODULE API (available if INTEGRALL_ENABLE_CAMERA is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_CAMERA_ENABLED

    /**
     * Pre-select a camera carrier board or custom pinout before calling begin()
     */
    void setCameraBoard(CameraBoard board) {
        _camera_module.setBoard(board);
    }

    void setCameraPins(const CameraPins& customPins) {
        _camera_module.setCustomPins(customPins);
    }

    /**
     * Start the camera hardware with default/pre-selected pins for the board
     */
    bool enableCamera() {
        return _camera_module.begin();
    }

    /**
     * Start (or re-configure) the camera hardware for a specific ESP32-CAM board model
     * @param board       CameraBoard enum (e.g., CAM_BOARD_AI_THINKER, CAM_BOARD_XIAO_ESP32S3, CAM_BOARD_FREENOVE_S3)
     * @param frameSize   Initial resolution (default: FRAMESIZE_QVGA, or FRAMESIZE_VGA / FRAMESIZE_SVGA / FRAMESIZE_UXGA)
     * @param jpegQuality JPEG compression (4 = highest quality, 63 = smallest file; -1 = auto)
     * @param xclkMhz     External clock in MHz (default: 20)
     */
    bool enableCamera(CameraBoard board, framesize_t frameSize = FRAMESIZE_QVGA, int jpegQuality = -1, int xclkMhz = 20) {
        return _camera_module.begin(board, frameSize, jpegQuality, xclkMhz);
    }

    /**
     * Start (or re-configure) the camera hardware with a custom GPIO pinout struct
     */
    bool enableCamera(const CameraPins& customPins, framesize_t frameSize = FRAMESIZE_QVGA, int jpegQuality = -1, int xclkMhz = 20) {
        return _camera_module.begin(customPins, frameSize, jpegQuality, xclkMhz);
    }

    /**
     * Manually start the MJPEG stream server (:80 UI and :81/stream)
     */
    void startCameraServer() {
        _camera_module.startServer();
    }

    /**
     * Get the stream URL (requires WiFi to be connected)
     */
    const char* getCameraStreamURL() {
        static char url[64];
        snprintf(url, sizeof(url), "http://%s:81/stream", getIPAddress());
        return url;
    }
    
    /**
     * Capture a single frame from the camera.
     * Use cameraRelease() to free the memory afterward.
     */
    camera_fb_t* cameraCapture() {
        return _camera_module.capture();
    }

    /**
     * Release a captured camera frame buffer.
     */
    void cameraRelease(camera_fb_t* fb) {
        _camera_module.release(fb);
    }

    /**
     * Runtime camera resolution, quality, orientation, image tuning & Flash LED controls
     */
    bool setCameraResolution(framesize_t frameSize)            { return _camera_module.setResolution(frameSize); }
    bool setCameraQuality(int quality)                         { return _camera_module.setQuality(quality); }
    bool setCameraFlip(bool vflip, bool hmirror = false)       { return _camera_module.setFlip(vflip, hmirror); }
    bool setCameraBrightness(int level)                        { return _camera_module.setBrightness(level); }
    bool setCameraContrast(int level)                          { return _camera_module.setContrast(level); }
    bool setCameraSaturation(int level)                        { return _camera_module.setSaturation(level); }
    bool setCameraEffect(int effect)                           { return _camera_module.setSpecialEffect(effect); }
    bool setCameraWhiteBalance(bool enable, int mode = 0)      { return _camera_module.setWhiteBalance(enable, mode); }
    void setCameraFlash(uint8_t intensity)                     { _camera_module.setFlash(intensity); }
    const char* getCameraSensorName() const                    { return _camera_module.getSensorName(); }
    const char* getCameraBoardName() const                     { return _camera_module.getBoardName(); }
    sensor_t* cameraSensor()                                   { return _camera_module.getSensor(); }

    #endif // INTEGRALL_MODULE_CAMERA_ENABLED


    // ========================================================================
    // PROJECT-LEVEL API: ALARM SYSTEM
    // Requires: SENSORS + RELAY + optionally BUZZER + LCD/OLED
    // ========================================================================
    #if INTEGRALL_MODULE_SENSORS_ENABLED && INTEGRALL_MODULE_RELAY_ENABLED

    /**
     * Configure a motion-triggered alarm system.
     * @param pirPin      PIR sensor GPIO pin
     * @param relayIndex  Relay to trigger (e.g., siren, light)
     * @param cooldownMs  Minimum time between alarm triggers (ms)
     */
    void alarmSetup(uint8_t pirPin, int relayIndex, uint32_t cooldownMs = 10000) {
        _alarm_pir         = pirPin;
        _alarm_relay       = relayIndex;
        _alarm_cooldown    = cooldownMs;
        _alarm_last_trig   = 0;
        _alarm_active      = true;
        pinMode(pirPin, INPUT);
        #if INTEGRALL_MODULE_LCD_ENABLED
        _lcd_module.print(" == ALARM SYS ==", 0, 0);
        _lcd_module.print("   Monitoring   ", 0, 1);
        #endif
    }

    /**
     * Call in loop() - detects motion, triggers alarm, shows feedback.
     */
    void alarmUpdate() {
        if (!_alarm_active) return;
        bool motion = _sensor_module.isTriggered(_alarm_pir);
        unsigned long now = millis();
        if (motion && (now - _alarm_last_trig > _alarm_cooldown)) {
            _alarm_last_trig = now;
            relayOn(_alarm_relay);
            relaySetTimeout(_alarm_relay, 5000);
            #if INTEGRALL_MODULE_LCD_ENABLED
            _lcd_module.print(" !! INTRUDER !! ", 0, 0);
            _lcd_module.print("  ALARM ON!     ", 0, 1);
            #endif
            #if INTEGRALL_MODULE_BUZZER_ENABLED
            _buzzer_module.alert();
            #endif
            _logEvent("alarm", "motion_detected");
        }
    }

    #endif // ALARM SYSTEM

    // ========================================================================
    // PROJECT-LEVEL API: PARKING SENSOR
    // Requires: SENSORS + optionally LCD/OLED + BUZZER
    // ========================================================================
    #if INTEGRALL_MODULE_SENSORS_ENABLED

    /**
     * Configure a parking / proximity display system.
     * @param trigPin    HC-SR04 trigger pin
     * @param echoPin    HC-SR04 echo pin
     * @param warnCM     Distance at which to warn (yellow)
     * @param stopCM     Distance at which to show STOP (red), 0=no stop
     */
    void parkingSetup(uint8_t trigPin, uint8_t echoPin,
                      float warnCM = 50.0f, float stopCM = 20.0f) {
        _park_trig  = trigPin;
        _park_echo  = echoPin;
        _park_warn  = warnCM;
        _park_stop  = stopCM;
        _park_active = true;
        #if INTEGRALL_MODULE_LCD_ENABLED
        _lcd_module.print(" PARKING SENSOR ", 0, 0);
        _lcd_module.print("   Ready...     ", 0, 1);
        #endif
    }

    /**
     * Call in loop() - reads distance and shows status on LCD / buzzer.
     */
    void parkingUpdate() {
        if (!_park_active) return;
        float d = _sensor_module.readDistanceCM(_park_trig, _park_echo, 2);
        if (d < 0) return;
        #if INTEGRALL_MODULE_LCD_ENABLED
        char buf[17];
        snprintf(buf, sizeof(buf), " Dist: %.1f cm   ", d);
        _lcd_module.print(buf, 0, 0);
        if (_park_stop > 0 && d <= _park_stop) {
            _lcd_module.print("  !! STOP !!    ", 0, 1);
            #if INTEGRALL_MODULE_BUZZER_ENABLED
            _buzzer_module.beep(80);
            #endif
        } else if (d <= _park_warn) {
            _lcd_module.print("  Slow down...  ", 0, 1);
        } else {
            _lcd_module.print("  Safe          ", 0, 1);
        }
        #endif
    }

    #endif // PARKING SENSOR

    // ========================================================================
    // PROJECT-LEVEL API: WEATHER STATION
    // Requires: SENSORS (with DHT) + optionally LCD/OLED
    // ========================================================================
    #if INTEGRALL_MODULE_SENSORS_ENABLED && INTEGRALL_DHT_AVAILABLE

    /**
     * Configure weather display + telemetry.
     * @param dhtPin        Data pin for DHT sensor
     * @param dhtType       DHT11 or DHT22
     * @param intervalSec   How often to read (seconds)
     */
    void weatherSetup(uint8_t dhtPin, uint8_t dhtType = 22, uint32_t intervalSec = 10) {
        _wx_pin      = dhtPin;
        _wx_type     = dhtType;
        _wx_interval = intervalSec * 1000UL;
        _wx_last     = 0;
        _wx_active   = true;
        #if INTEGRALL_MODULE_LCD_ENABLED
        _lcd_module.print("  WEATHER STA.  ", 0, 0);
        _lcd_module.print("  Loading...    ", 0, 1);
        #endif
    }

    /**
     * Call in loop() — reads temp/humidity on interval, displays and sends telemetry.
     */
    void weatherUpdate() {
        if (!_wx_active) return;
        unsigned long now = millis();
        if (now - _wx_last < _wx_interval) return;
        _wx_last = now;
        float t = _sensor_module.readTemperature(_wx_pin, _wx_type);
        float h = _sensor_module.readHumidity(_wx_pin, _wx_type);
        if (t == -999.0f || h < 0) {
            #if INTEGRALL_MODULE_LCD_ENABLED
            _lcd_module.print("! Sensor Error !", 0, 0);
            _lcd_module.print("  Check Wiring  ", 0, 1);
            #endif
            INTEGRALL_LOG_ERROR("DHT Sensor failed to read in WeatherStation update");
            return;
        }
        #if INTEGRALL_MODULE_LCD_ENABLED
        char row0[17], row1[17];
        snprintf(row0, sizeof(row0), " Temp: %.1fC     ", t);
        snprintf(row1, sizeof(row1), " Humi: %.0f%%     ", h);
        _lcd_module.print(row0, 0, 0);
        _lcd_module.print(row1, 0, 1);
        #endif
        // Auto-send to backend if online
        #if INTEGRALL_NETWORK_AVAILABLE
        if (isWiFiConnected() && _config.backend_url) {
            #if ARDUINOJSON_VERSION_MAJOR >= 7
            JsonDocument doc;
            #else
            StaticJsonDocument<128> doc;
            #endif
            doc["temperature"] = t;
            doc["humidity"] = h;
            sendTelemetry(doc);
        }
        #endif
        _logEvent("weather", "ok");
    }

    #endif // WEATHER STATION

    // ========================================================================
    // PROJECT-LEVEL API: SMART SWITCH (Motion Auto-On)
    // Requires: RELAY + SENSORS (PIR) + optional LCD/OLED
    // ========================================================================
    #if INTEGRALL_MODULE_RELAY_ENABLED && INTEGRALL_MODULE_SENSORS_ENABLED

    /**
     * Configure a motion-activated smart switch.
     * @param relayIndex     Relay to control
     * @param pirPin         PIR sensor pin
     * @param autoOffSec     Seconds after motion stops before turning off
     */
    void smartSwitchSetup(int relayIndex, uint8_t pirPin, uint32_t autoOffSec = 30) {
        _sw_relay      = relayIndex;
        _sw_pir        = pirPin;
        _sw_auto_off   = autoOffSec * 1000UL;
        _sw_last_motion = 0;
        _sw_active     = true;
        pinMode(pirPin, INPUT);
    }

    /**
     * Call in loop() — turns relay on with motion, off after auto-off timeout.
     */
    void smartSwitchUpdate() {
        if (!_sw_active) return;
        bool motion = _sensor_module.isTriggered(_sw_pir);
        unsigned long now = millis();
        if (motion) {
            _sw_last_motion = now;
            relayOn(_sw_relay);
        } else if (relayIsOn(_sw_relay) &&
                   (now - _sw_last_motion > _sw_auto_off)) {
            relayOff(_sw_relay);
            #if INTEGRALL_MODULE_LCD_ENABLED
            _lcd_module.print(" Smart Switch   ", 0, 0);
            _lcd_module.print(" Light OFF      ", 0, 1);
            #endif
        }
    }

    #endif // SMART SWITCH

    // ========================================================================
    
    /**
     * Check if system is fully online (WiFi + backend)
     */
    bool isOnline() const;
    
    /**
     * Check if WiFi is connected
     */
    bool isWiFiConnected() const;
    
    /**
     * Get device ID (unique per ESP32)
     */
    const char* getDeviceId() const;
    
    /**
     * Get assigned IP address
     */
    const char* getIPAddress() const;
    
    /**
     * Get WiFi signal strength (RSSI)
     */
    int getWiFiStrength() const;
    
    /**
     * Get current system state as string
     */
    const char* getStatusString() const;
    
    /**
     * Force reconnection to backend
     */
    void reconnect();
    
    #if INTEGRALL_NETWORK_AVAILABLE
    /**
     * Send telemetry data to backend
     * @param json_data JSON document with telemetry data
     */
    bool sendTelemetry(const JsonDocument& data);
    bool sendTelemetry(const char* key, float value);
    bool sendTelemetry(const char* key, const char* value);
    
    // ========================================================================
    // HTTP & HTTPS REST CLIENT API (GET, POST, PUT, PATCH, DELETE, TLS, HEADERS)
    // ========================================================================

    /**
     * Configure custom headers, Bearer Auth, TLS Root CA, or request timeout
     */
    void setHttpHeader(const char* key, const char* value) { _device_manager.setHttpHeader(key, value); }
    void clearHttpHeaders()                                { _device_manager.clearHttpHeaders(); }
    void setBearerToken(const char* token)                 { _device_manager.setBearerToken(token); }
    void setCACert(const char* rootCA)                     { _device_manager.setCACert(rootCA); }
    void setHttpInsecure(bool insecure = true)             { _device_manager.setHttpInsecure(insecure); }
    void setHttpTimeout(uint16_t timeoutMs)                { _device_manager.setHttpTimeout(timeoutMs); }

    /**
     * Perform a generic HTTP or HTTPS request returning a full HttpResponse struct
     */
    HttpResponse httpRequest(const char* method, const char* url, const char* payload = nullptr, const char* contentType = "application/json", const char* authToken = nullptr) {
        return _device_manager.httpRequest(method, url, payload, contentType, authToken);
    }

    /**
     * Perform an HTTP or HTTPS GET request (auto-detects http:// vs https://)
     * @return Response string (empty if failed)
     */
    String httpGet(const char* url, const char* authToken = nullptr) {
        return _device_manager.httpGet(url, authToken);
    }

    /**
     * Perform an HTTP or HTTPS POST request
     * @return HTTP status code (e.g. 200, 201)
     */
    int httpPost(const char* url, const char* payload, const char* contentType = "application/json", const char* authToken = nullptr) {
        return _device_manager.httpPost(url, payload, contentType, authToken);
    }

    /**
     * Perform an HTTP or HTTPS PUT request
     * @return HTTP status code
     */
    int httpPut(const char* url, const char* payload, const char* contentType = "application/json", const char* authToken = nullptr) {
        return _device_manager.httpPut(url, payload, contentType, authToken);
    }

    /**
     * Perform an HTTP or HTTPS PATCH request
     * @return HTTP status code
     */
    int httpPatch(const char* url, const char* payload, const char* contentType = "application/json", const char* authToken = nullptr) {
        return _device_manager.httpPatch(url, payload, contentType, authToken);
    }

    /**
     * Perform an HTTP or HTTPS DELETE request
     * @return HTTP status code
     */
    int httpDelete(const char* url, const char* payload = nullptr, const char* authToken = nullptr) {
        return _device_manager.httpDelete(url, payload, authToken);
    }
    
    /**
     * Post a file-like buffer as multipart form data (supports HTTP & HTTPS)
     */
    int httpPostFile(const char* url, uint8_t* data, size_t len, const char* fileName, const char* fileKey = "file", const char* extraKey = nullptr, const char* extraVal = nullptr) {
        return _device_manager.httpPostFile(url, data, len, fileName, fileKey, extraKey, extraVal);
    }

    /**
     * Get a network stream for raw reading (e.g. audio stream)
     */
    Stream* getStream(const char* url) {
        return _device_manager.getStream(url);
    }

    /**
     * Close the current network stream
     */
    void endStream() {
        _device_manager.endStream();
    }

    /**
     * Get the body or HTTP status code of the last HTTP/HTTPS response
     */
    const char* getLastResponse() const {
        return _device_manager.getLastResponse();
    }

    int getLastStatusCode() const {
        return _device_manager.getLastStatusCode();
    }
    #endif

    // ========================================================================
    // C++ HTML TAG & RESPONSIVE DASHBOARD RENDERER (Available on all boards)
    // ========================================================================
    HtmlBuilder& html() { return _html_builder; }
    String htmlTag(const char* tagName, const String& inner = "", const char* attrs = nullptr) { return HtmlBuilder::tag(tagName, inner, attrs); }
    String htmlVoidTag(const char* tagName, const char* attrs = nullptr)                       { return HtmlBuilder::voidTag(tagName, attrs); }
    String htmlCard(const String& title, const String& bodyHtml)                               { return HtmlBuilder::card(title, bodyHtml); }
    String htmlMetric(const String& label, const String& value, const char* unit = "", const char* id = nullptr) { return HtmlBuilder::metric(label, value, unit, id); }
    String htmlBadge(const String& text, const char* color = "cyan")                           { return HtmlBuilder::badge(text, color); }
    String htmlButton(const String& label, const char* endpoint, const char* color = "cyan")   { return HtmlBuilder::button(label, endpoint, color); }
    String htmlToggle(const String& label, const char* id, bool checked, const char* endpoint) { return HtmlBuilder::toggle(label, id, checked, endpoint); }
    String htmlSlider(const String& label, const char* id, int minV, int maxV, int curV, const char* endpoint)   { return HtmlBuilder::slider(label, id, minV, maxV, curV, endpoint); }
    String htmlProgressBar(const String& label, int percent, const char* id = nullptr)         { return HtmlBuilder::progressBar(label, percent, id); }
    String htmlWsScript(uint16_t wsPort = 81, const char* targetId = "ws-log")                 { return HtmlBuilder::wsScript(wsPort, targetId); }
    String htmlPage(const String& title, const String& bodyHtml, uint16_t refreshSec = 0, bool dark = true) { return HtmlBuilder::page(title, bodyHtml, refreshSec, dark); }

    // ========================================================================
    // LOCAL HTTP/REST WEB SERVER & WEBSOCKETS (ESP32 / ESP8266)
    // ========================================================================
    #if INTEGRALL_WEBSERVER_AVAILABLE
    bool   webServerBegin(uint16_t port = 80)                    { return _web_module.beginServer(port); }
    void   onGet(const char* uri, WebRouteCallback handler)      { _web_module.onGet(uri, handler); }
    void   onPost(const char* uri, WebRouteCallback handler)     { _web_module.onPost(uri, handler); }
    void   onPut(const char* uri, WebRouteCallback handler)      { _web_module.onPut(uri, handler); }
    void   onPatch(const char* uri, WebRouteCallback handler)    { _web_module.onPatch(uri, handler); }
    void   onDelete(const char* uri, WebRouteCallback handler)   { _web_module.onDelete(uri, handler); }
    void   servePage(const char* uri, const String& html)        { _web_module.servePage(uri, html); }
    void   serveJson(const char* uri, const String& json)        { _web_module.serveJson(uri, json); }
    String webArg(const char* name)                              { return _web_module.arg(name); }
    String webBody()                                             { return _web_module.body(); }
    void   webSendHtml(const String& html, int code = 200)       { _web_module.sendHtml(html, code); }
    void   webSendJson(const String& json, int code = 200)       { _web_module.sendJson(json, code); }
    void   webSendText(const String& text, int code = 200)       { _web_module.sendText(text, code); }
    void   webRedirect(const char* location, int code = 302)     { _web_module.sendRedirect(location, code); }
    #endif

    #if defined(ESP32) || defined(ESP8266)
    bool    wsServerBegin(uint16_t port = 81)                    { return _web_module.beginWsServer(port); }
    void    onWsMessage(WsMessageCallback cb)                    { _web_module.onWsMessage(cb); }
    uint8_t wsBroadcast(const char* message)                     { return _web_module.wsBroadcast(message); }
    uint8_t wsClientCount()                                      { return _web_module.wsClientCount(); }
    bool    wsConnect(const char* url)                           { return _web_module.wsConnect(url); }
    bool    wsConnect(const char* host, uint16_t port, const char* path = "/") { return _web_module.wsConnect(host, port, path); }
    bool    wsSend(const char* message)                          { return _web_module.wsSend(message); }
    void    onWsClientMessage(WsMessageCallback cb)              { _web_module.onWsClientMessage(cb); }
    bool    wsConnected()                                        { return _web_module.wsConnected(); }
    void    wsDisconnect()                                       { _web_module.wsDisconnect(); }
    #endif

    /**
     * Enable or disable automatic IoT event logging to backend
     */
    void enableEventLog(bool enabled) { _event_log_enabled = enabled; }

    /**
     * Start a non-blocking blink on a pin
     * @param pin      GPIO pin (e.g. 2 for ESP32 Built-in)
     * @param interval Time in ms between flips
     */
    void blink(uint8_t pin, uint32_t interval = 1000) {
        _blinker.begin(pin, interval);
    }

    /**
     * Stop the current blinker
     */
    void stopBlink() {
        _blinker.stop();
    }

private:
    DeviceManager _device_manager;
    DeviceConfig _config;
    bool _initialized;
    
    #if INTEGRALL_MODULE_RELAY_ENABLED
    RelayModule _relay_module;
    int _relay_count;
    #endif

    #if INTEGRALL_MODULE_LCD_ENABLED
    LCDModule _lcd_module;
    #endif

    #if INTEGRALL_MODULE_SENSORS_ENABLED || INTEGRALL_MODULE_IMU_ENABLED
    SensorModule _sensor_module;
    #endif

    #if INTEGRALL_MODULE_CELLULAR_ENABLED
    CellularModule _cellular_module;
    #endif

    #if INTEGRALL_MODULE_SERVO_ENABLED
    ServoModule _servo_module;
    #endif

    #if INTEGRALL_MODULE_KEYPAD_ENABLED
    KeypadModule _keypad_module;
    #endif

    #if INTEGRALL_MODULE_OLED_ENABLED
    OLEDModule _oled_module;
    #endif

    #if INTEGRALL_MODULE_BUZZER_ENABLED
    BuzzerModule _buzzer_module;
    #endif

    #if INTEGRALL_MODULE_RGB_ENABLED
    RGBModule _rgb_module;
    #endif

    #if INTEGRALL_MODULE_CAMERA_ENABLED
    CameraModule _camera_module;
    #endif

    #if INTEGRALL_MODULE_INPUT_ENABLED
    InputModule _input_module;
    #endif

    #if INTEGRALL_MODULE_STORAGE_ENABLED
    StorageModule _storage_module;
    #endif

    #if INTEGRALL_MODULE_TIME_ENABLED
    TimeModule _time_module;
    #endif

    #if INTEGRALL_MODULE_COMM_ENABLED
    CommunicationModule _comm_module;
    #endif

    #if INTEGRALL_MODULE_POWER_ENABLED
    PowerModule _power_module;
    #endif

    #if INTEGRALL_MODULE_AUDIO_ENABLED
    AudioModule _audio_module;
    #endif

    #if INTEGRALL_MODULE_STEPPER_ENABLED
    StepperModule _stepper_module;
    #endif

    // Lock system state
    #if INTEGRALL_MODULE_KEYPAD_ENABLED && INTEGRALL_MODULE_RELAY_ENABLED
    bool          _lock_active     = false;
    bool          _lock_locked_out = false;
    uint8_t       _lock_state      = 0; // 0: idle, 1: unlocked wait, 2: denied wait
    unsigned long _lock_state_ts   = 0;
    int           _lock_relay      = 0;
    uint32_t      _lock_unlock_ms  = 5000;
    uint8_t       _lock_max_tries  = 3;
    uint8_t       _lock_tries      = 0;
    char          _lock_pin[17]    = "";

    void _lockShowIdle() {
        #if INTEGRALL_MODULE_LCD_ENABLED
        _lcd_module.print("  === LOCK ===  ", 0, 0);
        _lcd_module.print("  Enter PIN:    ", 0, 1);
        #endif
    }
    #endif

    // Alarm System state
    #if INTEGRALL_MODULE_SENSORS_ENABLED && INTEGRALL_MODULE_RELAY_ENABLED
    bool          _alarm_active   = false;
    uint8_t       _alarm_pir      = 0;
    int           _alarm_relay    = 0;
    uint32_t      _alarm_cooldown = 10000;
    unsigned long _alarm_last_trig = 0;
    #endif

    // Parking Sensor state
    #if INTEGRALL_MODULE_SENSORS_ENABLED
    bool    _park_active = false;
    uint8_t _park_trig   = 0;
    uint8_t _park_echo   = 0;
    float   _park_warn   = 50.0f;
    float   _park_stop   = 20.0f;
    #endif

    // Weather Station state
    #if INTEGRALL_MODULE_SENSORS_ENABLED && INTEGRALL_DHT_AVAILABLE
    bool          _wx_active   = false;
    uint8_t       _wx_pin      = 0;
    uint8_t       _wx_type     = 22;
    uint32_t      _wx_interval = 10000;
    unsigned long _wx_last     = 0;
    #endif

    // Smart Switch state
    #if INTEGRALL_MODULE_RELAY_ENABLED && INTEGRALL_MODULE_SENSORS_ENABLED
    bool          _sw_active      = false;
    int           _sw_relay       = 0;
    uint8_t       _sw_pir         = 0;
    uint32_t      _sw_auto_off    = 30000;
    unsigned long _sw_last_motion = 0;
    #endif

    BlinkerModule _blinker;
    WebModule     _web_module;
    HtmlBuilder   _html_builder;
    bool _event_log_enabled = true;

    /**
     * Internal helper to log events to backend and serial
     */
    void _logEvent(const char* event_type, const char* detail) {
        (void)event_type;
        (void)detail;
        INTEGRALL_LOG_INFO_VAL("Event: ", event_type);
        #if INTEGRALL_NETWORK_AVAILABLE
        if (_event_log_enabled && isOnline()) {
            #if ARDUINOJSON_VERSION_MAJOR >= 7
            JsonDocument doc;
            #else
            StaticJsonDocument<128> doc;
            #endif
            doc["event"] = event_type;
            doc["detail"] = detail;
            sendTelemetry(doc);
        }
        #endif
    }

    void _beginModules();
    void _handleModules();
};

} // namespace Integrall

// ============================================================================
// IMPLEMENTATION
// ============================================================================

namespace Integrall {

System::System() 
    : _initialized(false)
    #if INTEGRALL_MODULE_RELAY_ENABLED
    , _relay_count(0)
    #endif
{
}

System::~System() {
}

void System::_beginModules() {
    #if INTEGRALL_MODULE_RELAY_ENABLED
    if (!_relay_module.begin(4)) {  // Support up to 4 relays
        INTEGRALL_LOG_ERROR("RelayModule initialization failed");
    }
    #endif

    #if INTEGRALL_MODULE_LCD_ENABLED
    if (!_lcd_module.begin()) {
        INTEGRALL_LOG_ERROR("LCDModule initialization failed");
    }
    #endif

    #if INTEGRALL_MODULE_OLED_ENABLED
    if (!_oled_module.begin()) {
        INTEGRALL_LOG_ERROR("OLEDModule initialization failed! Check I2C wiring.");
    } else {
        INTEGRALL_LOG_INFO("OLED Connected Successfully");
    }
    #endif

    #if INTEGRALL_MODULE_CAMERA_ENABLED
    if (!_camera_module.begin()) {
        INTEGRALL_LOG_ERROR("CameraModule initialization failed");
    }
    #endif
}

bool System::begin(const DeviceConfig& config) {
    // 0. The ABSOLUTE first step is starting the Serial port for debugging
    Logger::begin(115200); 
    
    _config = config;
    
    INTEGRALL_LOG_INFO("=== Integrall System Starting ===");
    
    // Initialize device manager (WiFi, backend)
    bool net_ok = _device_manager.begin(config);
    if (!net_ok) {
        INTEGRALL_LOG_WARN("DeviceManager initial connection failed; continuing in background");
    }
    
    // Always initialize enabled hardware modules even if WiFi is temporarily unreachable
    _beginModules();
    
    _initialized = true;
    INTEGRALL_LOG_INFO("=== Integrall System Ready ===");
    return net_ok;
}

void System::handle() {
    if (!_initialized) return;
    
    // Handle core device management
    _device_manager.handle();
    
    // Auto-update LCD with WiFi status only on state changes when WiFi is configured
    #if INTEGRALL_MODULE_LCD_ENABLED && INTEGRALL_MODULE_WIFI_ENABLED
    if (_config.wifi_ssid && strlen(_config.wifi_ssid) > 0 && !_config.use_wifi_manager) {
        static int _last_lcd_net_state = -1;
        int cur_state = !isWiFiConnected() ? 0 : (!isOnline() ? 1 : 2);
        if (cur_state != _last_lcd_net_state) {
            _last_lcd_net_state = cur_state;
            if (cur_state == 0) {
                _lcd_module.print("WiFi Discon...", 0, 1);
            } else if (cur_state == 1 && _config.backend_url && _config.api_key) {
                _lcd_module.print("Syncing Cloud..", 0, 1);
            }
        }
    }
    #endif

    // Handle modules
    _handleModules();
}

bool System::begin() {
    DeviceConfig defaultConfig;
    #ifdef INTEGRALL_DEFAULT_SSID
    defaultConfig.wifi_ssid = INTEGRALL_DEFAULT_SSID;
    #endif
    #ifdef INTEGRALL_DEFAULT_PASS
    defaultConfig.wifi_password = INTEGRALL_DEFAULT_PASS;
    #endif
    #ifdef INTEGRALL_DEFAULT_BACKEND_URL
    defaultConfig.backend_url = INTEGRALL_DEFAULT_BACKEND_URL;
    #endif
    #ifdef INTEGRALL_DEFAULT_API_KEY
    defaultConfig.api_key = INTEGRALL_DEFAULT_API_KEY;
    #endif
    return begin(defaultConfig);
}

void System::_handleModules() {
    #if INTEGRALL_MODULE_RELAY_ENABLED
    _relay_module.handle();
    #endif
    
    #if INTEGRALL_MODULE_BUZZER_ENABLED
    _buzzer_module.handle();
    #endif

    #if INTEGRALL_MODULE_LCD_ENABLED
    _lcd_module.handle();
    #endif

    #if INTEGRALL_MODULE_SERVO_ENABLED
    _servo_module.handle();
    #endif

    _blinker.handle();
    _web_module.handle();
}

// Relay Module implementations
#if INTEGRALL_MODULE_RELAY_ENABLED

int System::enableRelay(uint8_t pin, bool active_low, const char* name) {
    if (!_initialized) {
        INTEGRALL_LOG_ERROR("System not initialized");
        return -1;
    }
    
    RelayConfig config;
    config.pin = pin;
    config.active_low = active_low;
    config.name = name;
    config.safety_timeout_ms = 0;
    config.default_state = false;
    config.interlock_group = 0;
    
    if (_relay_module.addRelay(_relay_count, config)) {
        INTEGRALL_LOG_INFO_VAL("Relay enabled on pin ", pin);
        return _relay_count++;
    }
    
    return -1;
}

void System::relayOn(int relay_index) {
    if (relay_index >= 0 && relay_index < _relay_count) {
        _relay_module.on(relay_index);
    }
}

void System::relayOff(int relay_index) {
    if (relay_index >= 0 && relay_index < _relay_count) {
        _relay_module.off(relay_index);
    }
}

void System::relayToggle(int relay_index) {
    if (relay_index >= 0 && relay_index < _relay_count) {
        _relay_module.toggle(relay_index);
    }
}

void System::relaySetTimeout(int relay_index, uint32_t duration_ms) {
    if (relay_index >= 0 && relay_index < _relay_count) {
        _relay_module.setSafetyTimeout(relay_index, duration_ms);
    }
}

void System::relaySetInterlock(int relay_index, uint8_t group) {
    if (relay_index >= 0 && relay_index < _relay_count) {
        _relay_module.setInterlockGroup(relay_index, group);
    }
}

void System::relayAttachButton(int relay_index, uint8_t button_pin, bool active_low) {
    if (relay_index >= 0 && relay_index < _relay_count) {
        _relay_module.attachTrigger(relay_index, button_pin, active_low);
    }
}

bool System::relayIsOn(int relay_index) {
    if (relay_index >= 0 && relay_index < _relay_count) {
        return _relay_module.isOn(relay_index);
    }
    return false;
}

void System::allRelaysOff() {
    _relay_module.allOff();
}

#endif  // INTEGRALL_MODULE_RELAY_ENABLED

// System Status implementations
bool System::isOnline() const {
    return _device_manager.isOnline();
}

bool System::isWiFiConnected() const {
    return _device_manager.isWiFiConnected();
}

const char* System::getDeviceId() const {
    return _device_manager.getDeviceId();
}

const char* System::getIPAddress() const {
    return _device_manager.getDeviceIp();
}

int System::getWiFiStrength() const {
    return _device_manager.getWiFiStrength();
}

const char* System::getStatusString() const {
    switch (_device_manager.getState()) {
        case DeviceState::UNINITIALIZED: return "UNINITIALIZED";
        case DeviceState::CONFIGURING: return "CONFIGURING";
        case DeviceState::DISCONNECTED: return "DISCONNECTED";
        case DeviceState::CONNECTING_WIFI: return "CONNECTING_WIFI";
        case DeviceState::WIFI_CONNECTED: return "WIFI_CONNECTED";
        case DeviceState::REGISTERING: return "REGISTERING";
        case DeviceState::ONLINE: return "ONLINE";
        case DeviceState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void System::reconnect() {
    _device_manager.reconnect();
}

    #if INTEGRALL_NETWORK_AVAILABLE
    /**
     * Send a single numeric value to the cloud backend
     */
    bool System::sendTelemetry(const char* key, float value) {
        #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
        #else
        StaticJsonDocument<128> doc;
        #endif
        doc[key] = value;
        return _device_manager.sendTelemetry(doc);
    }

    /**
     * Send a single string value to the cloud backend
     */
    bool System::sendTelemetry(const char* key, const char* value) {
        #if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
        #else
        StaticJsonDocument<128> doc;
        #endif
        doc[key] = value;
        return _device_manager.sendTelemetry(doc);
    }

    /**
     * Send a full JSON document to the cloud backend
     */
    bool System::sendTelemetry(const JsonDocument& data) {
        return _device_manager.sendTelemetry(data);
    }
    #endif

} // namespace Integrall

#endif // INTEGRALL_H
