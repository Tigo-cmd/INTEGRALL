/**
 * IntegrallConfig.h
 * 
 * Central configuration for Integrall Framework.
 * Control which modules are compiled into your project.
 */

#ifndef INTEGRALL_CONFIG_H
#define INTEGRALL_CONFIG_H

#include <Arduino.h>

// Default debug level if not specified in sketch (0=none, 1=errors, 2=warnings, 3=info, 4=verbose)
#ifndef INTEGRALL_DEBUG_LEVEL
  #define INTEGRALL_DEBUG_LEVEL 3
#endif

#ifndef INTEGRALL_VERSION_STRING
  #define INTEGRALL_VERSION_STRING "1.0.0"
#endif

// ============================================================================
// CORE HELPER MACROS
// ============================================================================
#ifndef INTEGRALL_SAFE_STRCPY
  #define INTEGRALL_SAFE_STRCPY(dst, src, size) { strncpy(dst, src, size - 1); dst[size - 1] = '\0'; }
#endif

#ifndef INTEGRALL_HTTP_RETRY_ATTEMPTS
  #define INTEGRALL_HTTP_RETRY_ATTEMPTS 3
#endif

// ============================================================================
// MODULE SELECTION (Internal Flags)
// These are set by defining flags in your sketch BEFORE Integrall.h
// ============================================================================

// WiFi & Networking
#ifdef INTEGRALL_ENABLE_WIFI
  #define INTEGRALL_MODULE_WIFI_ENABLED 1
#else
  #define INTEGRALL_MODULE_WIFI_ENABLED 0
#endif

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

// Buzzer Module
#ifdef INTEGRALL_ENABLE_BUZZER
  #define INTEGRALL_MODULE_BUZZER_ENABLED 1
#else
  #define INTEGRALL_MODULE_BUZZER_ENABLED 0
#endif

// RGB LED Module
#ifdef INTEGRALL_ENABLE_RGB
  #define INTEGRALL_MODULE_RGB_ENABLED 1
#else
  #define INTEGRALL_MODULE_RGB_ENABLED 0
#endif

// Camera Module
#ifdef INTEGRALL_ENABLE_CAMERA
  #if !INTEGRALL_MODULE_WIFI_ENABLED
    #warning "Integrall: Camera requires INTEGRALL_ENABLE_WIFI to function. Disabling Camera."
    #define INTEGRALL_MODULE_CAMERA_ENABLED 0
  #else
    #define INTEGRALL_MODULE_CAMERA_ENABLED 1
    // Default to AI-Thinker if no other model is specified
  #if !defined(CAMERA_MODEL_WROVER_KIT) && !defined(CAMERA_MODEL_ESP_EYE) && \
      !defined(CAMERA_MODEL_M5STACK_PSRAM) && !defined(CAMERA_MODEL_AI_THINKER) && \
      !defined(CAMERA_MODEL_M5STACK_V2_PSRAM) && !defined(CAMERA_MODEL_M5STACK_WIDE) && \
      !defined(CAMERA_MODEL_M5STACK_ESP32CAM) && !defined(CAMERA_MODEL_M5STACK_UNITCAM) && \
      !defined(CAMERA_MODEL_M5STACK_CAMS3_UNIT) && !defined(CAMERA_MODEL_TTGO_T_JOURNAL) && \
      !defined(CAMERA_MODEL_XIAO_ESP32S3) && !defined(CAMERA_MODEL_ESP32_CAM_BOARD) && \
      !defined(CAMERA_MODEL_ESP32S3_CAM_LCD) && !defined(CAMERA_MODEL_ESP32S2_CAM_BOARD) && \
      !defined(CAMERA_MODEL_ESP32S3_EYE) && !defined(CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3) && \
      !defined(CAMERA_MODEL_DFRobot_Romeo_ESP32S3)
    #define CAMERA_MODEL_AI_THINKER
  #endif
  #endif
#else
  #define INTEGRALL_MODULE_CAMERA_ENABLED 0
#endif

#endif // INTEGRALL_CONFIG_H
