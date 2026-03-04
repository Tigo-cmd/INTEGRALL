/**
 * RelayModule.cpp
 * 
 * Relay control implementation with safety features
 */

#include "RelayModule.h"

namespace Integrall {

RelayModule::RelayModule()
    : _configs(nullptr)
    , _states(nullptr)
    , _trigger_pins(nullptr)
    , _trigger_active_low(nullptr)
    , _relay_count(0)
    , _max_relays(0)
    , _initialized(false)
    , _debounce_state(nullptr)
    , _debounce_last_time(nullptr) {
}

RelayModule::~RelayModule() {
    if (_configs) delete[] _configs;
    if (_states) delete[] _states;
    if (_trigger_pins) delete[] _trigger_pins;
    if (_trigger_active_low) delete[] _trigger_active_low;
    if (_debounce_state) delete[] _debounce_state;
    if (_debounce_last_time) delete[] _debounce_last_time;
}

bool RelayModule::begin(uint8_t max_relays) {
    if (_initialized) {
        INTEGRALL_LOG_WARN("RelayModule already initialized");
        return false;
    }
    
    _max_relays = max_relays;
    _configs = new RelayConfig[max_relays];
    _states = new RelayState[max_relays];
    _trigger_pins = new uint8_t[max_relays];
    _trigger_active_low = new bool[max_relays];
    _debounce_state = new uint8_t[max_relays];
    _debounce_last_time = new unsigned long[max_relays];
    
    // Initialize arrays
    for (uint8_t i = 0; i < max_relays; i++) {
        _configs[i] = {0, true, nullptr, 0, false, 0};
        _states[i] = {false, false, 0, 0, 0};
        _trigger_pins[i] = 255;  // Invalid pin
        _trigger_active_low[i] = true;
        _debounce_state[i] = 0;
        _debounce_last_time[i] = 0;
    }
    
    _initialized = true;
    INTEGRALL_LOG_INFO_VAL("RelayModule initialized, max relays: ", max_relays);
    return true;
}

bool RelayModule::addRelay(uint8_t index, const RelayConfig& config) {
    if (!_initialized || index >= _max_relays) {
        return false;
    }
    
    _configs[index] = config;
    
    // Setup pin
    pinMode(config.pin, OUTPUT);
    digitalWrite(config.pin, config.active_low ? HIGH : LOW);  // Start off
    
    // Initialize state
    _states[index].current_state = false;
    _states[index].target_state = false;
    _states[index].last_toggle_time = 0;
    _states[index].on_duration_ms = 0;
    _states[index].trigger_count = 0;
    
    if (_relay_count <= index) {
        _relay_count = index + 1;
    }
    
    INTEGRALL_LOG_INFO_VAL("Relay added at index ", index);
    if (config.name) {
        INTEGRALL_LOG_INFO_VAL("  Name: ", config.name);
    }
    
    return true;
}

bool RelayModule::addRelay(uint8_t pin, bool active_low, const char* name) {
    RelayConfig config;
    config.pin = pin;
    config.active_low = active_low;
    config.name = name;
    config.safety_timeout_ms = 0;
    config.default_state = false;
    config.interlock_group = 0;
    
    // Find first available index
    for (uint8_t i = 0; i < _max_relays; i++) {
        if (_configs[i].pin == 0) {  // Empty slot
            return addRelay(i, config);
        }
    }
    return false;
}

bool RelayModule::on(uint8_t index) {
    return setState(index, true);
}

bool RelayModule::off(uint8_t index) {
    return setState(index, false);
}

bool RelayModule::toggle(uint8_t index) {
    if (!isValid(index)) return false;
    return setState(index, !_states[index].current_state);
}

bool RelayModule::setState(uint8_t index, bool state) {
    if (!isValid(index)) {
        INTEGRALL_LOG_ERROR_VAL("Invalid relay index: ", index);
        return false;
    }
    
    // Check interlock if trying to turn on
    if (state && !_checkInterlock(index, true)) {
        INTEGRALL_LOG_WARN("Interlock prevented relay activation");
        return false;
    }
    
    // Update state tracking
    if (state != _states[index].current_state) {
        _states[index].target_state = state;
        _updatePhysicalState(index);
        
        // Update statistics
        _states[index].trigger_count++;
        if (state) {
            _states[index].last_toggle_time = millis();
        } else {
            // Accumulate on-time when turning off
            _states[index].on_duration_ms += (millis() - _states[index].last_toggle_time);
        }
        
        INTEGRALL_LOG_INFO_VAL("Relay ", index);
        INTEGRALL_LOG_INFO_VAL("  State: ", state ? "ON" : "OFF");
        
        // Handle interlock group - turn off others in same group
        if (state && _configs[index].interlock_group != 0) {
            for (uint8_t i = 0; i < _relay_count; i++) {
                if (i != index && 
                    _configs[i].interlock_group == _configs[index].interlock_group &&
                    _states[i].current_state) {
                    off(i);  // Turn off other relays in group
                }
            }
        }
    }
    
    return true;
}

void RelayModule::_updatePhysicalState(uint8_t index) {
    bool physical_state = _states[index].target_state;
    bool active_low = _configs[index].active_low;
    
    // Convert logical state to physical pin state
    uint8_t pin_value = physical_state ? (active_low ? LOW : HIGH) 
                                        : (active_low ? HIGH : LOW);
    
    digitalWrite(_configs[index].pin, pin_value);
    _states[index].current_state = _states[index].target_state;
}

bool RelayModule::_checkInterlock(uint8_t index, bool new_state) {
    if (!new_state) return true;  // Always allow turning off
    
    uint8_t group = _configs[index].interlock_group;
    if (group == 0) return true;  // No interlock
    
    // Check if any other relay in same group is on
    for (uint8_t i = 0; i < _relay_count; i++) {
        if (i != index && 
            _configs[i].interlock_group == group &&
            _states[i].current_state) {
            return false;  // Conflict detected
        }
    }
    
    return true;
}

bool RelayModule::isOn(uint8_t index) const {
    if (!isValid(index)) return false;
    return _states[index].current_state;
}

bool RelayModule::isValid(uint8_t index) const {
    return _initialized && index < _relay_count && _configs[index].pin != 0;
}

const char* RelayModule::getName(uint8_t index) const {
    if (!isValid(index)) return nullptr;
    return _configs[index].name;
}

void RelayModule::setSafetyTimeout(uint8_t index, uint32_t duration_ms) {
    if (isValid(index)) {
        _configs[index].safety_timeout_ms = duration_ms;
        INTEGRALL_LOG_INFO_VAL("Safety timeout set for relay ", index);
    }
}

void RelayModule::clearSafetyTimeout(uint8_t index) {
    setSafetyTimeout(index, 0);
}

void RelayModule::setInterlockGroup(uint8_t index, uint8_t group) {
    if (isValid(index)) {
        _configs[index].interlock_group = group;
        INTEGRALL_LOG_INFO_VAL("Interlock group set for relay ", index);
    }
}

bool RelayModule::attachTrigger(uint8_t relay_index, uint8_t trigger_pin, bool active_low) {
    if (!isValid(relay_index)) return false;
    
    _trigger_pins[relay_index] = trigger_pin;
    _trigger_active_low[relay_index] = active_low;
    
    pinMode(trigger_pin, INPUT_PULLUP);  // Assume internal pullup
    
    INTEGRALL_LOG_INFO_VAL("Trigger attached to relay ", relay_index);
    return true;
}

void RelayModule::checkTriggers() {
    static const unsigned long DEBOUNCE_MS = 50;  // 50ms debounce
    
    for (uint8_t i = 0; i < _relay_count; i++) {
        if (_trigger_pins[i] == 255) continue;  // No trigger configured
        
        bool raw_reading = digitalRead(_trigger_pins[i]);
        bool triggered = _trigger_active_low[i] ? !raw_reading : raw_reading;
        
        // Simple state machine debouncing
        unsigned long now = millis();
        
        if (triggered != (_debounce_state[i] & 0x01)) {
            // State changed, reset timer
            _debounce_last_time[i] = now;
            _debounce_state[i] = (_debounce_state[i] & 0xFE) | (triggered ? 1 : 0);
        }
        
        if ((now - _debounce_last_time[i]) >= DEBOUNCE_MS) {
            // Stable for debounce period
            if (triggered && !(_debounce_state[i] & 0x02)) {
                // Rising edge detected, toggle relay
                toggle(i);
                _debounce_state[i] |= 0x02;  // Mark as processed
            } else if (!triggered) {
                _debounce_state[i] &= ~0x02;  // Clear processed flag
            }
        }
    }
}

void RelayModule::handle() {
    _enforceSafetyTimeouts();
    checkTriggers();
}

void RelayModule::_enforceSafetyTimeouts() {
    unsigned long now = millis();
    
    for (uint8_t i = 0; i < _relay_count; i++) {
        if (!isValid(i)) continue;
        
        uint32_t timeout = _configs[i].safety_timeout_ms;
        if (timeout > 0 && _states[i].current_state) {
            unsigned long on_time = now - _states[i].last_toggle_time;
            if (on_time >= timeout) {
                INTEGRALL_LOG_WARN_VAL("Safety timeout expired for relay ", i);
                off(i);
            }
        }
    }
}

void RelayModule::allOff() {
    for (uint8_t i = 0; i < _relay_count; i++) {
        if (isValid(i)) {
            off(i);
        }
    }
}

void RelayModule::allOn() {
    for (uint8_t i = 0; i < _relay_count; i++) {
        if (isValid(i)) {
            on(i);
        }
    }
}

uint32_t RelayModule::getTotalOnTime(uint8_t index) const {
    if (!isValid(index)) return 0;
    
    uint32_t total = _states[index].on_duration_ms;
    if (_states[index].current_state) {
        total += (millis() - _states[index].last_toggle_time);
    }
    return total;
}

uint8_t RelayModule::getToggleCount(uint8_t index) const {
    if (!isValid(index)) return 0;
    return _states[index].trigger_count;
}

} // namespace Integrall
