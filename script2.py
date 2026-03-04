from script1 import base_dir


# Create the main Integrall.h - the user-facing API
integrall_main_h = """/**
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
    // SYSTEM STATUS API
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
    
    _initialized = true;
    INTEGRALL_LOG_INFO("=== Integrall System Ready ===");
    return true;
}

void System::handle() {
    if (!_initialized) return;
    
    // Handle core device management
    _device_manager.handle();
    
    // Handle modules
    _handleModules();
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
"""

with open(f"{base_dir}/firmware/Integrall/src/Integrall.h", "w") as f:
    f.write(integrall_main_h)

print("✅ Created main Integrall.h")
print("- Unified user-facing API")
print("- Conditional module compilation")
print("- Fluent interface design")
print("- Automatic initialization chaining")
