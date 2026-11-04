/**
 * SmartCamera.ino - Universal ESP32-CAM Implementation
 * 
 * Supports 19+ ESP32 / ESP32-S2 / ESP32-S3 Camera Boards & Ribbon Sensors
 * (OV2640, OV3660, OV5640, GC2145, etc.) with automatic orientation & color tuning:
 * - CAM_BOARD_AI_THINKER         (Default ESP32-CAM)
 * - CAM_BOARD_XIAO_ESP32S3       (Seeed Studio XIAO ESP32S3 Sense)
 * - CAM_BOARD_FREENOVE_S3        (Freenove ESP32-S3-WROOM CAM)
 * - CAM_BOARD_WROVER_KIT         (ESP-WROVER-KIT / Freenove WROVER CAM)
 * - CAM_BOARD_ESP_EYE / CAM_BOARD_ESP32S3_EYE
 * - CAM_BOARD_M5STACK_WIDE / CAM_BOARD_M5STACK_UNITCAM / CAM_BOARD_M5STACK_CAMS3_UNIT
 * - CAM_BOARD_TTGO_T_JOURNAL / CAM_BOARD_LILYGO_T_CAMERA_PLUS / CAM_BOARD_LILYGO_T_SIMCAM
 */

#define INTEGRALL_ENABLE_WIFI
#define INTEGRALL_ENABLE_CAMERA
#define INTEGRALL_DEBUG_LEVEL 3 // Enable INFO logging for debugging
#include <Integrall.h>

Integrall::System integrall;

void setup() {
    // Optional: Pre-select your camera board if not using AI-Thinker ESP32-CAM
    // integrall.setCameraBoard(CAM_BOARD_XIAO_ESP32S3);

    // 1. One-line WiFi initialization
    integrall.begin("YourWiFiName", "YourWiFiPassword");

    // 2. One-line Camera Setup (Pass board model & resolution, or leave empty for defaults)
    if (integrall.enableCamera(CAM_BOARD_AI_THINKER, FRAMESIZE_VGA)) {
        // Optional runtime adjustments:
        // integrall.setCameraFlip(true, false); // Flip vertically if mounted upside down
        // integrall.setCameraQuality(10);       // 4 (best quality) to 63 (fastest stream)
        // integrall.setCameraFlash(32);         // 0-255 onboard Flash LED brightness

        integrall.startCameraServer();
        Serial.print(">> Integrall: Camera Ready! Board: ");
        Serial.print(integrall.getCameraBoardName());
        Serial.print(" | Sensor: ");
        Serial.println(integrall.getCameraSensorName());
        Serial.print(">> Stream URL: ");
        Serial.println(integrall.getCameraStreamURL());
    } else {
        Serial.println(">> Integrall: Camera Init Failed.");
    }
}

void loop() {
    // 3. Keep the system alive (WiFi recovery, Cloud commands)
    integrall.handle();
}
