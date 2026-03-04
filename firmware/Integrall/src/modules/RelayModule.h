/**
 * RelayModule.h
 * 
 * Relay control module for Integrall Framework
 * 
 * Features:
 * - Safe relay control with state validation
 * - Optional safety timeout (auto-off after duration)
 * - Safety interlocks (prevent simultaneous activation of conflicting relays)
 * - Debounced manual trigger inputs
 * - State persistence across reconnections
 */

#ifndef INTEGRALL_RELAY_MODULE_H
#define INTEGRALL_RELAY_MODULE_H

#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"
#include <Arduino.h>

namespace Integrall {

// Relay configuration structure
struct RelayConfig {
    uint8_t pin;                    // GPIO pin number
    bool active_low;               // true if relay triggers on LOW (common for optocoupled modules)
    const char* name;              // Human-readable identifier
    uint32_t safety_timeout_ms;    // Auto-off after this duration (0 = disabled)
    bool default_state;            // State on initialization (usually false = off)
    uint8_t interlock_group;       // Relays in same group cannot be on simultaneously (0 = no interlock)
};

// Relay state structure
struct RelayState {
    bool current_state;
    bool target_state;
    unsigned long last_toggle_time;
    unsigned long on_duration_ms;
    uint8_t trigger_count;         // For debouncing
};

class RelayModule {
public:
    RelayModule();
    ~RelayModule();
    
    // Initialize the module - call in setup
    bool begin(uint8_t max_relays = 4);
    
    // Add a relay configuration
    bool addRelay(uint8_t index, const RelayConfig& config);
    
    // Convenience method for quick setup
    bool addRelay(uint8_t pin, bool active_low = true, const char* name = nullptr);
    
    // Control methods
    bool on(uint8_t index);
    bool off(uint8_t index);
    bool toggle(uint8_t index);
    bool setState(uint8_t index, bool state);
    
    // Get state
    bool isOn(uint8_t index) const;
    bool isValid(uint8_t index) const;
    const char* getName(uint8_t index) const;
    
    // Safety features
    void setSafetyTimeout(uint8_t index, uint32_t duration_ms);
    void clearSafetyTimeout(uint8_t index);
    void setInterlockGroup(uint8_t index, uint8_t group);
    
    // Manual trigger (for physical buttons) with debouncing
    bool attachTrigger(uint8_t relay_index, uint8_t trigger_pin, bool active_low = true);
    void checkTriggers();  // Call in loop for software debouncing
    
    // Maintenance - call in loop
    void handle();  // Handles safety timeouts and state updates
    
    // Bulk operations
    void allOff();
    void allOn();  // Use with caution!
    
    // Get relay count
    uint8_t getCount() const { return _relay_count; }
    
    // Statistics
    uint32_t getTotalOnTime(uint8_t index) const;  // Total time relay has been on (milliseconds)
    uint8_t getToggleCount(uint8_t index) const;     // Number of state changes

private:
    RelayConfig* _configs;
    RelayState* _states;
    uint8_t* _trigger_pins;        // For manual button inputs
    bool* _trigger_active_low;
    uint8_t _relay_count;
    uint8_t _max_relays;
    bool _initialized;
    
    // Internal methods
    void _updatePhysicalState(uint8_t index);
    bool _checkInterlock(uint8_t index, bool new_state);
    void _enforceSafetyTimeouts();
    bool _debounceRead(uint8_t index);
    
    // Debounce state for triggers
    uint8_t* _debounce_state;
    unsigned long* _debounce_last_time;
};

} // namespace Integrall

#endif // INTEGRALL_RELAY_MODULE_H
