#ifndef CAMERA_PINS_H
#define CAMERA_PINS_H

#include <stdint.h>

// Track whether the user explicitly defined a CAMERA_MODEL_* macro before including Integrall
#if defined(CAMERA_MODEL_WROVER_KIT) || defined(CAMERA_MODEL_ESP_EYE) || \
    defined(CAMERA_MODEL_M5STACK_PSRAM) || defined(CAMERA_MODEL_AI_THINKER) || \
    defined(CAMERA_MODEL_M5STACK_V2_PSRAM) || defined(CAMERA_MODEL_M5STACK_WIDE) || \
    defined(CAMERA_MODEL_M5STACK_ESP32CAM) || defined(CAMERA_MODEL_M5STACK_UNITCAM) || \
    defined(CAMERA_MODEL_M5STACK_CAMS3_UNIT) || defined(CAMERA_MODEL_TTGO_T_JOURNAL) || \
    defined(CAMERA_MODEL_XIAO_ESP32S3) || defined(CAMERA_MODEL_ESP32_CAM_BOARD) || \
    defined(CAMERA_MODEL_ESP32S3_CAM_LCD) || defined(CAMERA_MODEL_ESP32S2_CAM_BOARD) || \
    defined(CAMERA_MODEL_ESP32S3_EYE) || defined(CAMERA_MODEL_FREENOVE_ESP32S3_CAM) || \
    defined(CAMERA_MODEL_LILYGO_T_CAMERA_PLUS) || defined(CAMERA_MODEL_LILYGO_T_SIMCAM) || \
    defined(CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3) || defined(CAMERA_MODEL_DFRobot_Romeo_ESP32S3)
  #define INTEGRALL_USER_DEFINED_CAMERA_MODEL 1
#else
  #define INTEGRALL_USER_DEFINED_CAMERA_MODEL 0
  #define CAMERA_MODEL_AI_THINKER
#endif

// ============================================================================
// RUNTIME CAMERA BOARD PROFILES & CUSTOM PINOUT STRUCT
// ============================================================================
namespace Integrall {

enum CameraBoard : uint8_t {
    CAM_BOARD_AI_THINKER = 0,         // Standard AI-Thinker ESP32-CAM (OV2640 / OV3660 / OV5640)
    CAM_BOARD_WROVER_KIT,             // ESP-WROVER-KIT / Freenove ESP32-WROVER CAM
    CAM_BOARD_FREENOVE_S3,            // Freenove ESP32-S3-WROOM CAM
    CAM_BOARD_XIAO_ESP32S3,           // Seeed Studio XIAO ESP32S3 Sense
    CAM_BOARD_ESP_EYE,                // Espressif ESP-EYE
    CAM_BOARD_ESP32S3_EYE,            // Espressif ESP32-S3-EYE
    CAM_BOARD_M5STACK_PSRAM,          // M5Stack M5Camera (Model A)
    CAM_BOARD_M5STACK_V2_PSRAM,       // M5Stack M5Camera (Model B)
    CAM_BOARD_M5STACK_WIDE,           // M5Stack M5Camera Wide
    CAM_BOARD_M5STACK_ESP32CAM,       // M5Stack ESP32Cam (no PSRAM)
    CAM_BOARD_M5STACK_UNITCAM,        // M5Stack UnitCam
    CAM_BOARD_M5STACK_CAMS3_UNIT,     // M5Stack UnitCamS3 (ESP32-S3)
    CAM_BOARD_TTGO_T_JOURNAL,         // LilyGO TTGO T-Journal
    CAM_BOARD_LILYGO_T_CAMERA_PLUS,   // LilyGO T-Camera Plus
    CAM_BOARD_LILYGO_T_SIMCAM,        // LilyGO T-SimCam (ESP32-S3)
    CAM_BOARD_DFROBOT_FIREBEETLE2_S3, // DFRobot FireBeetle 2 / Romeo ESP32-S3
    CAM_BOARD_ESP32_CAM_BOARD,        // Espressif ESP32-CAM_BOARD
    CAM_BOARD_ESP32S2_CAM_BOARD,      // Espressif ESP32S2-CAM_BOARD
    CAM_BOARD_ESP32S3_CAM_LCD,        // Espressif ESP32S3-CAM-LCD
    CAM_BOARD_CUSTOM                  // Custom user-supplied CameraPins struct
};

struct CameraPins {
    int8_t pwdn;
    int8_t reset;
    int8_t xclk;
    int8_t siod;
    int8_t sioc;
    int8_t y9;
    int8_t y8;
    int8_t y7;
    int8_t y6;
    int8_t y5;
    int8_t y4;
    int8_t y3;
    int8_t y2;
    int8_t vsync;
    int8_t href;
    int8_t pclk;
    int8_t led;
    bool   defaultVFlip;
    bool   defaultHMirror;
    const char* boardName;
};

/**
 * Resolve the default CameraBoard based on any compile-time #define CAMERA_MODEL_* macro
 */
inline CameraBoard getDefaultCameraBoard() {
#if defined(CAMERA_MODEL_WROVER_KIT)
    return CAM_BOARD_WROVER_KIT;
#elif defined(CAMERA_MODEL_FREENOVE_ESP32S3_CAM)
    return CAM_BOARD_FREENOVE_S3;
#elif defined(CAMERA_MODEL_XIAO_ESP32S3)
    return CAM_BOARD_XIAO_ESP32S3;
#elif defined(CAMERA_MODEL_ESP_EYE)
    return CAM_BOARD_ESP_EYE;
#elif defined(CAMERA_MODEL_ESP32S3_EYE)
    return CAM_BOARD_ESP32S3_EYE;
#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
    return CAM_BOARD_M5STACK_PSRAM;
#elif defined(CAMERA_MODEL_M5STACK_V2_PSRAM)
    return CAM_BOARD_M5STACK_V2_PSRAM;
#elif defined(CAMERA_MODEL_M5STACK_WIDE)
    return CAM_BOARD_M5STACK_WIDE;
#elif defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    return CAM_BOARD_M5STACK_ESP32CAM;
#elif defined(CAMERA_MODEL_M5STACK_UNITCAM)
    return CAM_BOARD_M5STACK_UNITCAM;
#elif defined(CAMERA_MODEL_M5STACK_CAMS3_UNIT)
    return CAM_BOARD_M5STACK_CAMS3_UNIT;
#elif defined(CAMERA_MODEL_TTGO_T_JOURNAL)
    return CAM_BOARD_TTGO_T_JOURNAL;
#elif defined(CAMERA_MODEL_LILYGO_T_CAMERA_PLUS)
    return CAM_BOARD_LILYGO_T_CAMERA_PLUS;
#elif defined(CAMERA_MODEL_LILYGO_T_SIMCAM)
    return CAM_BOARD_LILYGO_T_SIMCAM;
#elif defined(CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3) || defined(CAMERA_MODEL_DFRobot_Romeo_ESP32S3)
    return CAM_BOARD_DFROBOT_FIREBEETLE2_S3;
#elif defined(CAMERA_MODEL_ESP32_CAM_BOARD)
    return CAM_BOARD_ESP32_CAM_BOARD;
#elif defined(CAMERA_MODEL_ESP32S2_CAM_BOARD)
    return CAM_BOARD_ESP32S2_CAM_BOARD;
#elif defined(CAMERA_MODEL_ESP32S3_CAM_LCD)
    return CAM_BOARD_ESP32S3_CAM_LCD;
#else
    return CAM_BOARD_AI_THINKER;
#endif
}

/**
 * Retrieve the complete GPIO mapping, Flash LED pin, and default orientation for a CameraBoard
 */
inline CameraPins getCameraPins(CameraBoard board) {
    switch (board) {
        case CAM_BOARD_WROVER_KIT:
            return { -1, -1, 21, 26, 27, 35, 34, 39, 36, 19, 18, 5, 4, 25, 23, 22, 2, false, false, "WROVER-KIT / Freenove WROVER" };
        case CAM_BOARD_FREENOVE_S3:
            return { -1, -1, 15, 4, 5, 16, 17, 18, 12, 10, 8, 9, 11, 6, 7, 13, 48, true, false, "Freenove ESP32-S3 CAM" };
        case CAM_BOARD_XIAO_ESP32S3:
            return { -1, -1, 10, 40, 39, 48, 11, 12, 14, 16, 18, 17, 15, 38, 47, 13, 21, false, false, "Seeed XIAO ESP32S3 Sense" };
        case CAM_BOARD_ESP_EYE:
            return { -1, -1, 4, 18, 23, 36, 37, 38, 39, 35, 14, 13, 34, 5, 27, 25, 22, false, false, "Espressif ESP-EYE" };
        case CAM_BOARD_ESP32S3_EYE:
            return { -1, -1, 15, 4, 5, 16, 17, 18, 12, 10, 8, 9, 11, 6, 7, 13, -1, true, false, "Espressif ESP32-S3-EYE" };
        case CAM_BOARD_M5STACK_PSRAM:
            return { -1, 15, 27, 25, 23, 19, 36, 18, 39, 5, 34, 35, 32, 22, 26, 21, 14, false, false, "M5Stack M5Camera (PSRAM)" };
        case CAM_BOARD_M5STACK_V2_PSRAM:
            return { -1, 15, 27, 22, 23, 19, 36, 18, 39, 5, 34, 35, 32, 25, 26, 21, -1, false, false, "M5Stack M5Camera V2" };
        case CAM_BOARD_M5STACK_WIDE:
            return { -1, 15, 27, 22, 23, 19, 36, 18, 39, 5, 34, 35, 32, 25, 26, 21, 2, true, true, "M5Stack Wide Camera" };
        case CAM_BOARD_M5STACK_ESP32CAM:
            return { -1, 15, 27, 25, 23, 19, 36, 18, 39, 5, 34, 35, 17, 22, 26, 21, -1, true, true, "M5Stack ESP32Cam" };
        case CAM_BOARD_M5STACK_UNITCAM:
            return { -1, 15, 27, 25, 23, 19, 36, 18, 39, 5, 34, 35, 32, 22, 26, 21, 4, false, false, "M5Stack UnitCam" };
        case CAM_BOARD_M5STACK_CAMS3_UNIT:
            return { -1, 21, 11, 17, 41, 13, 4, 10, 5, 7, 16, 15, 6, 42, 18, 12, 14, false, false, "M5Stack UnitCamS3" };
        case CAM_BOARD_TTGO_T_JOURNAL:
            return { 0, 15, 27, 25, 23, 19, 36, 18, 39, 5, 34, 35, 17, 22, 26, 21, -1, false, false, "LilyGO TTGO T-Journal" };
        case CAM_BOARD_LILYGO_T_CAMERA_PLUS:
            return { -1, -1, 4, 18, 23, 36, 37, 38, 39, 35, 26, 13, 34, 5, 27, 25, -1, false, false, "LilyGO T-Camera Plus" };
        case CAM_BOARD_LILYGO_T_SIMCAM:
            return { -1, 18, 14, 4, 5, 15, 16, 17, 12, 10, 8, 9, 11, 6, 7, 13, -1, false, false, "LilyGO T-SimCam S3" };
        case CAM_BOARD_DFROBOT_FIREBEETLE2_S3:
            return { -1, -1, 45, 1, 2, 48, 46, 8, 7, 4, 41, 40, 39, 6, 42, 5, -1, false, false, "DFRobot FireBeetle2 ESP32-S3" };
        case CAM_BOARD_ESP32_CAM_BOARD:
            return { 32, 33, 4, 18, 23, 36, 19, 21, 39, 35, 14, 13, 34, 5, 27, 25, -1, false, false, "Espressif ESP32-CAM_BOARD" };
        case CAM_BOARD_ESP32S2_CAM_BOARD:
            return { 1, 2, 42, 41, 18, 16, 39, 40, 15, 13, 5, 12, 14, 38, 4, 3, -1, false, false, "Espressif ESP32S2-CAM_BOARD" };
        case CAM_BOARD_ESP32S3_CAM_LCD:
            return { -1, -1, 40, 17, 18, 39, 41, 42, 12, 3, 14, 47, 13, 21, 38, 11, -1, false, false, "Espressif ESP32S3-CAM-LCD" };
        case CAM_BOARD_AI_THINKER:
        default:
            return { 32, -1, 0, 26, 27, 35, 34, 39, 36, 21, 19, 18, 5, 25, 23, 22, 4, false, false, "AI-Thinker ESP32-CAM" };
    }
}

} // namespace Integrall

// Expose board enum constants in global namespace for convenience
using Integrall::CameraBoard;
using Integrall::CameraPins;
using Integrall::CAM_BOARD_AI_THINKER;
using Integrall::CAM_BOARD_WROVER_KIT;
using Integrall::CAM_BOARD_FREENOVE_S3;
using Integrall::CAM_BOARD_XIAO_ESP32S3;
using Integrall::CAM_BOARD_ESP_EYE;
using Integrall::CAM_BOARD_ESP32S3_EYE;
using Integrall::CAM_BOARD_M5STACK_PSRAM;
using Integrall::CAM_BOARD_M5STACK_V2_PSRAM;
using Integrall::CAM_BOARD_M5STACK_WIDE;
using Integrall::CAM_BOARD_M5STACK_ESP32CAM;
using Integrall::CAM_BOARD_M5STACK_UNITCAM;
using Integrall::CAM_BOARD_M5STACK_CAMS3_UNIT;
using Integrall::CAM_BOARD_TTGO_T_JOURNAL;
using Integrall::CAM_BOARD_LILYGO_T_CAMERA_PLUS;
using Integrall::CAM_BOARD_LILYGO_T_SIMCAM;
using Integrall::CAM_BOARD_DFROBOT_FIREBEETLE2_S3;
using Integrall::CAM_BOARD_ESP32_CAM_BOARD;
using Integrall::CAM_BOARD_ESP32S2_CAM_BOARD;
using Integrall::CAM_BOARD_ESP32S3_CAM_LCD;
using Integrall::CAM_BOARD_CUSTOM;

// ============================================================================
// LEGACY COMPILE-TIME PIN MACROS (For 100% backward compatibility)
// ============================================================================
#if defined(CAMERA_MODEL_WROVER_KIT)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  21
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    19
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    5
#define Y2_GPIO_NUM    4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

#elif defined(CAMERA_MODEL_ESP_EYE)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  4
#define SIOD_GPIO_NUM  18
#define SIOC_GPIO_NUM  23

#define Y9_GPIO_NUM    36
#define Y8_GPIO_NUM    37
#define Y7_GPIO_NUM    38
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    35
#define Y4_GPIO_NUM    14
#define Y3_GPIO_NUM    13
#define Y2_GPIO_NUM    34
#define VSYNC_GPIO_NUM 5
#define HREF_GPIO_NUM  27
#define PCLK_GPIO_NUM  25

#define LED_GPIO_NUM 22

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM  27
#define SIOD_GPIO_NUM  25
#define SIOC_GPIO_NUM  23

#define Y9_GPIO_NUM    19
#define Y8_GPIO_NUM    36
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    5
#define Y4_GPIO_NUM    34
#define Y3_GPIO_NUM    35
#define Y2_GPIO_NUM    32
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM  26
#define PCLK_GPIO_NUM  21

#elif defined(CAMERA_MODEL_M5STACK_V2_PSRAM)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM  27
#define SIOD_GPIO_NUM  22
#define SIOC_GPIO_NUM  23

#define Y9_GPIO_NUM    19
#define Y8_GPIO_NUM    36
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    5
#define Y4_GPIO_NUM    34
#define Y3_GPIO_NUM    35
#define Y2_GPIO_NUM    32
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  26
#define PCLK_GPIO_NUM  21

#elif defined(CAMERA_MODEL_M5STACK_WIDE)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM  27
#define SIOD_GPIO_NUM  22
#define SIOC_GPIO_NUM  23

#define Y9_GPIO_NUM    19
#define Y8_GPIO_NUM    36
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    5
#define Y4_GPIO_NUM    34
#define Y3_GPIO_NUM    35
#define Y2_GPIO_NUM    32
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  26
#define PCLK_GPIO_NUM  21

#define LED_GPIO_NUM 2

#elif defined(CAMERA_MODEL_M5STACK_ESP32CAM)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM  27
#define SIOD_GPIO_NUM  25
#define SIOC_GPIO_NUM  23

#define Y9_GPIO_NUM    19
#define Y8_GPIO_NUM    36
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    5
#define Y4_GPIO_NUM    34
#define Y3_GPIO_NUM    35
#define Y2_GPIO_NUM    17
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM  26
#define PCLK_GPIO_NUM  21

#elif defined(CAMERA_MODEL_M5STACK_UNITCAM)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM  27
#define SIOD_GPIO_NUM  25
#define SIOC_GPIO_NUM  23

#define Y9_GPIO_NUM    19
#define Y8_GPIO_NUM    36
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    5
#define Y4_GPIO_NUM    34
#define Y3_GPIO_NUM    35
#define Y2_GPIO_NUM    32
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM  26
#define PCLK_GPIO_NUM  21

#elif defined(CAMERA_MODEL_M5STACK_CAMS3_UNIT)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM 21
#define XCLK_GPIO_NUM  11
#define SIOD_GPIO_NUM  17
#define SIOC_GPIO_NUM  41

#define Y9_GPIO_NUM    13
#define Y8_GPIO_NUM    4
#define Y7_GPIO_NUM    10
#define Y6_GPIO_NUM    5
#define Y5_GPIO_NUM    7
#define Y4_GPIO_NUM    16
#define Y3_GPIO_NUM    15
#define Y2_GPIO_NUM    6
#define VSYNC_GPIO_NUM 42
#define HREF_GPIO_NUM  18
#define PCLK_GPIO_NUM  12

#define LED_GPIO_NUM 14

#elif defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// 4 for flash led or 33 for normal led
#define LED_GPIO_NUM   4

#elif defined(CAMERA_MODEL_TTGO_T_JOURNAL)
#define PWDN_GPIO_NUM  0
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM  27
#define SIOD_GPIO_NUM  25
#define SIOC_GPIO_NUM  23

#define Y9_GPIO_NUM    19
#define Y8_GPIO_NUM    36
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    5
#define Y4_GPIO_NUM    34
#define Y3_GPIO_NUM    35
#define Y2_GPIO_NUM    17
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM  26
#define PCLK_GPIO_NUM  21

#elif defined(CAMERA_MODEL_XIAO_ESP32S3)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39

#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

#elif defined(CAMERA_MODEL_ESP32_CAM_BOARD)
#define USE_BOARD_HEADER 0
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   33
#define XCLK_GPIO_NUM    4
#define SIOD_GPIO_NUM    18
#define SIOC_GPIO_NUM    23

#define Y9_GPIO_NUM 36
#define Y8_GPIO_NUM 19
#define Y7_GPIO_NUM 21
#define Y6_GPIO_NUM 39
#if USE_BOARD_HEADER
#define Y5_GPIO_NUM 13
#else
#define Y5_GPIO_NUM 35
#endif
#define Y4_GPIO_NUM 14
#if USE_BOARD_HEADER
#define Y3_GPIO_NUM 35
#else
#define Y3_GPIO_NUM 13
#endif
#define Y2_GPIO_NUM    34
#define VSYNC_GPIO_NUM 5
#define HREF_GPIO_NUM  27
#define PCLK_GPIO_NUM  25

#elif defined(CAMERA_MODEL_ESP32S3_CAM_LCD)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  40
#define SIOD_GPIO_NUM  17
#define SIOC_GPIO_NUM  18

#define Y9_GPIO_NUM    39
#define Y8_GPIO_NUM    41
#define Y7_GPIO_NUM    42
#define Y6_GPIO_NUM    12
#define Y5_GPIO_NUM    3
#define Y4_GPIO_NUM    14
#define Y3_GPIO_NUM    47
#define Y2_GPIO_NUM    13
#define VSYNC_GPIO_NUM 21
#define HREF_GPIO_NUM  38
#define PCLK_GPIO_NUM  11

#elif defined(CAMERA_MODEL_ESP32S2_CAM_BOARD)
#define USE_BOARD_HEADER 0
#define PWDN_GPIO_NUM    1
#define RESET_GPIO_NUM   2
#define XCLK_GPIO_NUM    42
#define SIOD_GPIO_NUM    41
#define SIOC_GPIO_NUM    18

#define Y9_GPIO_NUM 16
#define Y8_GPIO_NUM 39
#define Y7_GPIO_NUM 40
#define Y6_GPIO_NUM 15
#if USE_BOARD_HEADER
#define Y5_GPIO_NUM 12
#else
#define Y5_GPIO_NUM 13
#endif
#define Y4_GPIO_NUM 5
#if USE_BOARD_HEADER
#define Y3_GPIO_NUM 13
#else
#define Y3_GPIO_NUM 12
#endif
#define Y2_GPIO_NUM    14
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  4
#define PCLK_GPIO_NUM  3

#elif defined(CAMERA_MODEL_ESP32S3_EYE) || defined(CAMERA_MODEL_FREENOVE_ESP32S3_CAM)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5

#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13

#elif defined(CAMERA_MODEL_LILYGO_T_CAMERA_PLUS)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  4
#define SIOD_GPIO_NUM  18
#define SIOC_GPIO_NUM  23

#define Y9_GPIO_NUM    36
#define Y8_GPIO_NUM    37
#define Y7_GPIO_NUM    38
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    35
#define Y4_GPIO_NUM    26
#define Y3_GPIO_NUM    13
#define Y2_GPIO_NUM    34
#define VSYNC_GPIO_NUM 5
#define HREF_GPIO_NUM  27
#define PCLK_GPIO_NUM  25

#elif defined(CAMERA_MODEL_LILYGO_T_SIMCAM)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM 18
#define XCLK_GPIO_NUM  14
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5

#define Y9_GPIO_NUM    15
#define Y8_GPIO_NUM    16
#define Y7_GPIO_NUM    17
#define Y6_GPIO_NUM    12
#define Y5_GPIO_NUM    10
#define Y4_GPIO_NUM    8
#define Y3_GPIO_NUM    9
#define Y2_GPIO_NUM    11
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13

#elif defined(CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3) || defined(CAMERA_MODEL_DFRobot_Romeo_ESP32S3)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  45
#define SIOD_GPIO_NUM  1
#define SIOC_GPIO_NUM  2

#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    46
#define Y7_GPIO_NUM    8
#define Y6_GPIO_NUM    7
#define Y5_GPIO_NUM    4
#define Y4_GPIO_NUM    41
#define Y3_GPIO_NUM    40
#define Y2_GPIO_NUM    39
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  42
#define PCLK_GPIO_NUM  5

#else
#error "Camera model not selected"
#endif
#endif // CAMERA_PINS_H
