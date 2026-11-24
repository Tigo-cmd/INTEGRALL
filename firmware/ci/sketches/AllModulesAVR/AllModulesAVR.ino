// CI compile-coverage sketch: the offline module set an Arduino Uno can hold.
#define INTEGRALL_ENABLE_RELAY
#define INTEGRALL_ENABLE_LCD
#define INTEGRALL_ENABLE_SENSORS
#define INTEGRALL_ENABLE_SERVO
#define INTEGRALL_ENABLE_KEYPAD
#define INTEGRALL_ENABLE_BUZZER
#define INTEGRALL_ENABLE_RGB
#define INTEGRALL_ENABLE_STORAGE
#define INTEGRALL_ENABLE_STEPPER
#include <Integrall.h>
#include "allmodules_body.inc"  // found via -I set in ci/platformio.ini
