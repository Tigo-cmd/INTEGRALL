// CI compile-coverage sketch: every module that runs on ESP32 (camera excluded; see CameraOnly).
#define INTEGRALL_ENABLE_WIFI
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_LCD
#define INTEGRALL_ENABLE_OLED
#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_IMU
#define INTEGRALL_ENABLE_SERVO
#define INTEGRALL_ENABLE_KEYPAD
#define INTEGRALL_ENABLE_BUZZER
#define INTEGRALL_ENABLE_RGB
#define INTEGRALL_ENABLE_INPUT
#define INTEGRALL_ENABLE_STORAGE
#define INTEGRALL_ENABLE_CELLULAR
#define INTEGRALL_ENABLE_TIME
#define INTEGRALL_ENABLE_COMM
#define INTEGRALL_ENABLE_POWER
#define INTEGRALL_ENABLE_AUDIO
#define INTEGRALL_ENABLE_STEPPER
#include <Integrall.h>
#include "allmodules_body.inc"  // found via -I set in ci/platformio.ini
