/**
 * RelayModule.h
 *
 * Relay control module for the Integrall Framework (header-only, no heap).
 *
 * Features:
 *  - Active-low / active-high relay boards
 *  - Safety timeout: auto-off after N ms
 *  - Interlock groups: while one relay of a group is ON, turning another relay
 *    of the same group ON is REFUSED (returns false). This is the safe choice
 *    for H-bridge / motor direction relays: you must switch the first one off
 *    explicitly (break-before-make), never get both on at once.
 *  - Debounced physical toggle buttons
 *  - Per-relay statistics (total on-time, toggle count)
 *
 * Capacity: INTEGRALL_MAX_RELAYS (default 8; 4 on AVR). Any GPIO, including GPIO 0.
 */

#ifndef INTEGRALL_RELAY_MODULE_H
#define INTEGRALL_RELAY_MODULE_H

#include "../config/IntegrallConfig.h"
#include "../core/Logger.h"
#include <Arduino.h>

#ifndef INTEGRALL_MAX_RELAYS
  #if defined(__AVR__)
    #define INTEGRALL_MAX_RELAYS 4   // Uno/Nano RAM budget
  #else
    #define INTEGRALL_MAX_RELAYS 8
  #endif
#endif

namespace Integrall {

// Relay configuration structure
struct RelayConfig {
    uint8_t     pin = 0;                 // GPIO pin number
    bool        active_low = true;       // true if relay triggers on LOW (common optocoupled boards)
    const char* name = nullptr;          // Human-readable identifier
    uint32_t    safety_timeout_ms = 0;   // Auto-off after this duration (0 = disabled)
    bool        default_state = false;   // State applied when the relay is added
    uint8_t     interlock_group = 0;     // Relays in same non-zero group can't be ON together
};

class RelayModule {
public:
    static const uint8_t NO_PIN = 0xFF;

    RelayModule() {}

    /** Kept for API compatibility. Capacity is fixed at compile time. */
    bool begin(uint8_t max_relays = INTEGRALL_MAX_RELAYS) {
        if (max_relays > INTEGRALL_MAX_RELAYS) {
            INTEGRALL_LOG_WARN_VAL("RelayModule: capacity is INTEGRALL_MAX_RELAYS = ", INTEGRALL_MAX_RELAYS);
        }
        return true;
    }

    /** Add (or replace) the relay at a given index. */
    bool addRelay(uint8_t index, const RelayConfig& config) {
        if (index >= INTEGRALL_MAX_RELAYS) {
            INTEGRALL_LOG_ERROR_VAL("RelayModule: index out of range: ", index);
            return false;
        }
        Slot& s = _slots[index];
        s = Slot();
        s.cfg = config;
        s.used = true;
        pinMode(config.pin, OUTPUT);
        _write(s, false);
        if (index >= _count) _count = index + 1;
        if (config.default_state) on(index);
        INTEGRALL_LOG_INFO_VAL("Relay added at index ", index);
        return true;
    }

    /** Add a relay in the first free slot. @return index, or -1 if full. */
    int addRelay(uint8_t pin, bool active_low = true, const char* name = nullptr) {
        for (uint8_t i = 0; i < INTEGRALL_MAX_RELAYS; i++) {
            if (!_slots[i].used) {
                RelayConfig c;
                c.pin = pin; c.active_low = active_low; c.name = name;
                return addRelay(i, c) ? (int)i : -1;
            }
        }
        INTEGRALL_LOG_ERROR("RelayModule: all slots used (raise INTEGRALL_MAX_RELAYS)");
        return -1;
    }

    bool on(uint8_t index)     { return setState(index, true); }
    bool off(uint8_t index)    { return setState(index, false); }
    bool toggle(uint8_t index) { return isValid(index) && setState(index, !_slots[index].on); }

    /**
     * Set the logical state. Returns false if the index is invalid or an
     * interlock refused the activation. Setting the current state is a no-op
     * that returns true.
     */
    bool setState(uint8_t index, bool state) {
        if (!isValid(index)) {
            INTEGRALL_LOG_ERROR_VAL("Invalid relay index: ", index);
            return false;
        }
        Slot& s = _slots[index];
        if (state == s.on) return true;
        if (state && !_interlockAllows(index)) {
            INTEGRALL_LOG_WARN_VAL("Interlock refused activation of relay ", index);
            return false;
        }
        const unsigned long now = millis();
        if (state) {
            s.on_since = now;
        } else {
            s.on_total_ms += now - s.on_since;
        }
        s.toggles++;
        _write(s, state);
        if (state) { INTEGRALL_LOG_INFO_VAL("Relay ON: ", index); }
        else       { INTEGRALL_LOG_INFO_VAL("Relay OFF: ", index); }
        return true;
    }

    bool isOn(uint8_t index) const     { return isValid(index) && _slots[index].on; }
    bool isValid(uint8_t index) const  { return index < INTEGRALL_MAX_RELAYS && _slots[index].used; }
    const char* getName(uint8_t index) const { return isValid(index) ? _slots[index].cfg.name : nullptr; }
    uint8_t getCount() const { return _count; }

    void setSafetyTimeout(uint8_t index, uint32_t duration_ms) {
        if (isValid(index)) _slots[index].cfg.safety_timeout_ms = duration_ms;
    }
    void clearSafetyTimeout(uint8_t index) { setSafetyTimeout(index, 0); }
    uint32_t getSafetyTimeout(uint8_t index) const { return isValid(index) ? _slots[index].cfg.safety_timeout_ms : 0; }

    void setInterlockGroup(uint8_t index, uint8_t group) {
        if (isValid(index)) _slots[index].cfg.interlock_group = group;
    }

    /** Attach a push-button that toggles the relay (debounced, press edge). */
    bool attachTrigger(uint8_t relay_index, uint8_t trigger_pin, bool active_low = true) {
        if (!isValid(relay_index)) return false;
        Slot& s = _slots[relay_index];
        s.btn_pin = trigger_pin;
        s.btn_active_low = active_low;
        pinMode(trigger_pin, active_low ? INPUT_PULLUP : INPUT);
        s.btn_raw = s.btn_stable = _readButton(s);
        s.btn_changed_at = millis();
        return true;
    }

    /** Call every loop (System::handle does this). Never blocks. */
    void handle() {
        const unsigned long now = millis();
        for (uint8_t i = 0; i < _count; i++) {
            Slot& s = _slots[i];
            if (!s.used) continue;

            // Safety timeout
            if (s.on && s.cfg.safety_timeout_ms > 0 && (now - s.on_since) >= s.cfg.safety_timeout_ms) {
                INTEGRALL_LOG_WARN_VAL("Safety timeout expired for relay ", i);
                off(i);
            }

            // Debounced button
            if (s.btn_pin != NO_PIN) {
                const bool raw = _readButton(s);
                if (raw != s.btn_raw) { s.btn_raw = raw; s.btn_changed_at = now; }
                if (raw != s.btn_stable && (now - s.btn_changed_at) >= DEBOUNCE_MS) {
                    s.btn_stable = raw;
                    if (raw) toggle(i);   // act on press edge only
                }
            }
        }
    }

    /** Kept for API compatibility; handle() already does this. */
    void checkTriggers() { handle(); }

    void allOff() { for (uint8_t i = 0; i < _count; i++) if (isValid(i)) off(i); }
    /** Turns every relay on that its interlock group allows. */
    void allOn()  { for (uint8_t i = 0; i < _count; i++) if (isValid(i)) on(i); }

    uint32_t getTotalOnTime(uint8_t index) const {
        if (!isValid(index)) return 0;
        const Slot& s = _slots[index];
        return s.on_total_ms + (s.on ? (millis() - s.on_since) : 0);
    }
    uint32_t getToggleCount(uint8_t index) const { return isValid(index) ? _slots[index].toggles : 0; }

private:
    static const unsigned long DEBOUNCE_MS = 50;

    struct Slot {
        RelayConfig   cfg;
        bool          used = false;
        bool          on = false;
        unsigned long on_since = 0;
        uint32_t      on_total_ms = 0;
        uint32_t      toggles = 0;
        uint8_t       btn_pin = NO_PIN;
        bool          btn_active_low = true;
        bool          btn_raw = false;
        bool          btn_stable = false;
        unsigned long btn_changed_at = 0;
    };

    Slot    _slots[INTEGRALL_MAX_RELAYS];
    uint8_t _count = 0;

    void _write(Slot& s, bool state) {
        s.on = state;
        digitalWrite(s.cfg.pin, (state != s.cfg.active_low) ? HIGH : LOW);
    }

    bool _readButton(const Slot& s) const {
        const bool high = digitalRead(s.btn_pin) == HIGH;
        return s.btn_active_low ? !high : high;
    }

    bool _interlockAllows(uint8_t index) const {
        const uint8_t g = _slots[index].cfg.interlock_group;
        if (g == 0) return true;
        for (uint8_t i = 0; i < _count; i++) {
            if (i != index && _slots[i].used && _slots[i].on && _slots[i].cfg.interlock_group == g) return false;
        }
        return true;
    }
};

} // namespace Integrall

#endif // INTEGRALL_RELAY_MODULE_H
