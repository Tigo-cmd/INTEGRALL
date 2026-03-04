/**
 * IntegrallConfig.h
 * 
 * Central configuration for Integrall Framework.
 * Control which modules are compiled into your project.
 */

#ifndef INTEGRALL_CONFIG_H
#define INTEGRALL_CONFIG_H

#include <Arduino.h>

// ============================================================================
// MODULE SELECTION (Internal Flags)
// These are set by defining flags in your sketch BEFORE Integrall.h
// ============================================================================

// Relay Module
#ifdef INTEGRALL_ENABLE_RELAY
  #define INTEGRALL_MODULE_RELAY_ENABLED 1
#else
  #define INTEGRALL_MODULE_RELAY_ENABLED 0
#endif

// Sensor Modules (DHT11/22, Analog, HC-SR04)
#ifdef INTEGRALL_ENABLE_SENSORS
  #define INTEGRALL_MODULE_SENSORS_ENABLED 1
#else
  #define INTEGRALL_MODULE_SENSORS_ENABLED 0
#endif

// Servo Control
#ifdef INTEGRALL_ENABLE_SERVO
  #define INTEGRALL_MODULE_SERVO_ENABLED 1
#else
  #define INTEGRALL_MODULE_SERVO_ENABLED 0
#endif

// LCD Module (Simplified I2C)
#ifdef INTEGRALL_ENABLE_LCD
  #define INTEGRALL_MODULE_LCD_ENABLED 1
#else
  #define INTEGRALL_MODULE_LCD_ENABLED 0
#endif

// OLED Module
#ifdef INTEGRALL_ENABLE_OLED
  #define INTEGRALL_MODULE_OLED_ENABLED 1
#else
  #define INTEGRALL_MODULE_OLED_ENABLED 0
#endif

// Keypad Module
#ifdef INTEGRALL_ENABLE_KEYPAD
  #define INTEGRALL_MODULE_KEYPAD_ENABLED 1
#else
  #define INTEGRALL_MODULE_KEYPAD_ENABLED 0
#endif

// ============================================================================
// MORE COMPLEX SYSTEM CONFIGURATION
// ============================================================================

struct DeviceConfig {
    const char* wifi_ssid;
    const char* wifi_password;
    const char* backend_url;
    const char* api_key;
    uint32_t poll_interval_ms = 5000;
};

#endif // INTEGRALL_CONFIG_H
