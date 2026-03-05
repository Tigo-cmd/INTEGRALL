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

#if INTEGRALL_MODULE_SENSORS_ENABLED
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

// ============================================================================
// MAIN SYSTEM CLASS
// ============================================================================

namespace Integrall {

class System {
public:
    System();
    ~System();
    
    /**
     * Initialize the Integrall system
     * @param config Device configuration (WiFi, backend, etc.)
     * @return true if initialization successful
     */
    bool begin(const DeviceConfig& config);
    
    /**
     * Main loop handler - call this in your loop() function
     * Handles WiFi, backend communication, module updates
     */
    void handle();
    
    /**
     * Start the system without WiFi (for basic Arduino boards)
     */
    bool begin();
    
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
     * Turn all relays off
     */
    void allRelaysOff();
    
    #endif  // INTEGRALL_MODULE_RELAY_ENABLED
    
    // ========================================================================
    // LCD MODULE API (available if INTEGRALL_ENABLE_LCD is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_LCD_ENABLED
    
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
    
    /**
     * Easy proximity alert
     * Returns true if object is closer than threshold
     */
    bool isNear(uint8_t trig, uint8_t echo, float threshold_cm) {
        return _sensor_module.isWithinRange(trig, echo, 0, threshold_cm);
    }
    
    /**
     * Read analog input as a percentage (0-100)
     */
    int readAnalogPercent(uint8_t pin) {
        return _sensor_module.readAnalogPercent(pin);
    }
    
    /**
     * Read digital sensor like PIR Motion
     */
    bool isTriggered(uint8_t pin) {
        return _sensor_module.isTriggered(pin);
    }
    
    #endif // INTEGRALL_MODULE_SENSORS_ENABLED
    
    // ========================================================================
    // SERVO MODULE API (available if INTEGRALL_ENABLE_SERVO is defined)
    // ========================================================================
    #if INTEGRALL_MODULE_SERVO_ENABLED
    
    /**
     * Control a servo motor
     * @param pin GPIO pin for PWM
     * @param angle Angle to set (0-180)
     */
    void setServo(uint8_t pin, uint8_t angle) {
        _servo_module.attach(pin);
        _servo_module.set(angle);
    }
    
    /**
     * Map a pot or sensor directly to a servo
     */
    void setServoFromAnalog(uint8_t servo_pin, uint8_t analog_pin) {
        _servo_module.attach(servo_pin);
        _servo_module.setFromAnalog(analog_pin);
    }
    
    /**
     * Start a non-blocking sweep of the servo
     */
    void sweepServo(uint8_t pin, uint32_t speed_ms = 15) {
        _servo_module.attach(pin);
        _servo_module.updateSweep(speed_ms);
    }
    
    #endif // INTEGRALL_MODULE_SERVO_ENABLED

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
        strncpy(_lock_pin, pin, sizeof(_lock_pin) - 1);
        _lock_relay      = relayIndex;
        _lock_unlock_ms  = unlockMs;
        _lock_max_tries  = maxRetries;
        _lock_tries      = 0;
        _lock_locked_out = false;
        _lock_active     = true;
        relaySetTimeout(relayIndex, unlockMs);
        _lockShowIdle();
    }

    /**
     * Call this in loop() to run the entire lock system.
     * Handles: key capture, asterisk display, PIN check,
     *          LCD feedback, relay control, and lockout logic.
     */
    void lockUpdate() {
        if (!_lock_active) return;

        if (_lock_locked_out) {
            // Stay locked out — do nothing until reset
            return;
        }

        // Capture keypresses and show asterisk feedback on LCD
        const char* input = _keypad_module.captureString(8);

        #if INTEGRALL_MODULE_LCD_ENABLED
        // Show asterisks on row 1, padded with spaces
        char stars[17];
        uint8_t len = strlen(input);
        for (uint8_t i = 0; i < 16; i++)
            stars[i] = (i < len) ? '*' : ' ';
        stars[16] = '\0';
        _lcd_module.print(stars, 0, 1);
        #endif

        // When '#' is pressed, evaluate the PIN
        if (_keypad_module.isSubmitted()) {
            if (_keypad_module.checkPin(_lock_pin)) {
                // ✅ Correct PIN
                _lock_tries = 0;
                #if INTEGRALL_MODULE_LCD_ENABLED
                _lcd_module.print("  ** UNLOCKED **", 0, 0);
                _lcd_module.print("   Welcome!     ", 0, 1);
                #endif
                relayOn(_lock_relay);
                delay(_lock_unlock_ms);
                _lockShowIdle();
            } else {
                // ❌ Wrong PIN
                _lock_tries++;
                if (_lock_max_tries > 0 && _lock_tries >= _lock_max_tries) {
                    // Lockout activated
                    _lock_locked_out = true;
                    #if INTEGRALL_MODULE_LCD_ENABLED
                    _lcd_module.print(" !! LOCKED OUT !", 0, 0);
                    _lcd_module.print("  Call Admin    ", 0, 1);
                    #endif
                } else {
                    #if INTEGRALL_MODULE_LCD_ENABLED
                    char msg[17];
                    snprintf(msg, sizeof(msg), " Wrong! (%d left)",
                             _lock_max_tries - _lock_tries);
                    _lcd_module.print("  !! DENIED !!  ", 0, 0);
                    _lcd_module.print(msg, 0, 1);
                    #endif
                    delay(2000);
                    _lockShowIdle();
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

    #endif // INTEGRALL_MODULE_OLED_ENABLED

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
    
    /**
     * Send telemetry data to backend
     * @param json_data JSON document with telemetry data
     */
    bool sendTelemetry(const JsonDocument& data);

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

    #if INTEGRALL_MODULE_SENSORS_ENABLED
    SensorModule _sensor_module;
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

    // Lock system state
    #if INTEGRALL_MODULE_KEYPAD_ENABLED && INTEGRALL_MODULE_RELAY_ENABLED
    bool     _lock_active     = false;
    bool     _lock_locked_out = false;
    int      _lock_relay      = 0;
    uint32_t _lock_unlock_ms  = 5000;
    uint8_t  _lock_max_tries  = 3;
    uint8_t  _lock_tries      = 0;
    char     _lock_pin[17]    = "";

    void _lockShowIdle() {
        #if INTEGRALL_MODULE_LCD_ENABLED
        _lcd_module.print("  === LOCK ===  ", 0, 0);
        _lcd_module.print("  Enter PIN:   ", 0, 1);
        #endif
    }
    #endif

    void _handleModules();
    void _dispatchCommand(const char* command_type, const JsonObject& params);
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

bool System::begin(const DeviceConfig& config) {
    _config = config;
    
    INTEGRALL_LOG_INFO("=== Integrall System Starting ===");
    
    // Initialize device manager (WiFi, backend)
    if (!_device_manager.begin(config)) {
        INTEGRALL_LOG_ERROR("DeviceManager initialization failed");
        return false;
    }
    
    // Initialize enabled modules
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
    
    _initialized = true;
    INTEGRALL_LOG_INFO("=== Integrall System Ready ===");
    return true;
}

void System::handle() {
    if (!_initialized) return;
    
    // Handle core device management
    _device_manager.handle();
    
    // Auto-update LCD with WiFi status if enabled
    #if INTEGRALL_MODULE_LCD_ENABLED
    static unsigned long _last_status_update = 0;
    if (millis() - _last_status_update > 5000) {
        if (!isWiFiConnected()) {
            _lcd_module.print("WiFi Discon...", 0, 1);
        } else if (!isOnline()) {
            _lcd_module.print("Backend Error", 0, 1);
        }
        _last_status_update = millis();
    }
    #endif
    
    // Handle modules
    _handleModules();
}

bool System::begin() {
    INTEGRALL_LOG_INFO("=== Integrall Module Initialization ===");
    
    #if INTEGRALL_MODULE_LCD_ENABLED
    if (!_lcd_module.begin()) {
        INTEGRALL_LOG_ERROR("LCDModule initialization failed");
    }
    #endif

    _initialized = true;
    return true;
}

void System::_handleModules() {
    #if INTEGRALL_MODULE_RELAY_ENABLED
    _relay_module.handle();
    #endif
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
    return _device_manager.getWiFiRSSI();
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

bool System::sendTelemetry(const JsonDocument& data) {
    return _device_manager.sendTelemetry(data);
}

} // namespace Integrall

#endif // INTEGRALL_H
