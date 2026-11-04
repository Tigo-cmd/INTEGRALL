#ifndef INTEGRALL_CAMERA_MODULE_H
#define INTEGRALL_CAMERA_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_CAMERA_ENABLED
#include "esp_camera.h"
#include <WiFi.h>
#include "camera/camera_pins.h"

// External declarations for the server and flash logic in app_httpd.cpp
void startCameraServer();
void setupLedFlash();
void setupLedFlash(int pin);
void setCameraFlashDuty(int duty);

#include "../core/Logger.h"

namespace Integrall {

class CameraModule {
public:
    CameraModule()
        : _initialized(false),
          _server_started(false),
          _board(getDefaultCameraBoard()),
          _pins(getCameraPins(_board)),
          _frame_size(FRAMESIZE_QVGA),
          _jpeg_quality(-1),
          _xclk_mhz(20) {}

    /**
     * Pre-configure the camera board profile before calling begin()
     */
    void setBoard(CameraBoard board) {
        _board = board;
        _pins = getCameraPins(board);
    }

    /**
     * Pre-configure a custom camera pinout before calling begin()
     */
    void setCustomPins(const CameraPins& customPins) {
        _board = CAM_BOARD_CUSTOM;
        _pins = customPins;
        if (!_pins.boardName) _pins.boardName = "Custom ESP32-CAM";
    }

    /**
     * Start the camera hardware with default/pre-selected board settings
     */
    bool begin() {
        if (_initialized) return true;
        return _initWithPins(_pins, _frame_size, _jpeg_quality, _xclk_mhz);
    }

    /**
     * Start (or re-configure) the camera hardware for a specific ESP32-CAM board model
     * @param board       CameraBoard enum (e.g., CAM_BOARD_AI_THINKER, CAM_BOARD_XIAO_ESP32S3, CAM_BOARD_FREENOVE_S3)
     * @param frameSize   Initial resolution (default: FRAMESIZE_QVGA for fast AI/HTTP, or FRAMESIZE_VGA / FRAMESIZE_SVGA)
     * @param jpegQuality JPEG compression (4 = highest quality, 63 = smallest file; -1 = auto 10 with PSRAM / 12 without)
     * @param xclkMhz     External clock frequency in MHz (default: 20 MHz)
     */
    bool begin(CameraBoard board, framesize_t frameSize = FRAMESIZE_QVGA, int jpegQuality = -1, int xclkMhz = 20) {
        if (_initialized) {
            if (_board == board) {
                setResolution(frameSize);
                if (jpegQuality >= 4 && jpegQuality <= 63) setQuality(jpegQuality);
                return true;
            }
            esp_camera_deinit();
            _initialized = false;
        }
        _board = board;
        _pins = getCameraPins(board);
        _frame_size = frameSize;
        _jpeg_quality = jpegQuality;
        _xclk_mhz = xclkMhz;
        return _initWithPins(_pins, _frame_size, _jpeg_quality, _xclk_mhz);
    }

    /**
     * Start (or re-configure) the camera hardware with a custom GPIO pinout struct
     */
    bool begin(const CameraPins& customPins, framesize_t frameSize = FRAMESIZE_QVGA, int jpegQuality = -1, int xclkMhz = 20) {
        if (_initialized) {
            esp_camera_deinit();
            _initialized = false;
        }
        setCustomPins(customPins);
        _frame_size = frameSize;
        _jpeg_quality = jpegQuality;
        _xclk_mhz = xclkMhz;
        return _initWithPins(_pins, _frame_size, _jpeg_quality, _xclk_mhz);
    }

    /**
     * Start the MJPEG stream server (:80 UI & :81/stream)
     */
    void startServer() {
        if (!_initialized || _server_started) return;

        setupLedFlash(_pins.led);
        startCameraServer();
        _server_started = true;
    }

    bool isReady() const { return _initialized; }
    bool isServerRunning() const { return _server_started; }

    /**
     * Capture a single frame from the camera.
     * Caller is responsible for calling release() to free the buffer.
     */
    camera_fb_t* capture() {
        if (!_initialized) return nullptr;
        return esp_camera_fb_get();
    }

    /**
     * Free the frame buffer returned by capture().
     */
    void release(camera_fb_t* fb) {
        if (fb) esp_camera_fb_return(fb);
    }

    // ========================================================================
    // RUNTIME SENSOR & FLASH SETTINGS
    // ========================================================================

    /**
     * Change camera frame resolution at runtime
     * Common values: FRAMESIZE_QVGA (320x240), FRAMESIZE_VGA (640x480),
     *                FRAMESIZE_SVGA (800x600), FRAMESIZE_HD (1280x720),
     *                FRAMESIZE_UXGA (1600x1200)
     */
    bool setResolution(framesize_t frameSize) {
        _frame_size = frameSize;
        if (!_initialized) return true;
        sensor_t* s = esp_camera_sensor_get();
        if (!s) return false;
        return (s->set_framesize(s, frameSize) == 0);
    }

    /**
     * Change JPEG compression quality at runtime (4 = best quality, 63 = lowest quality/fastest)
     */
    bool setQuality(int quality) {
        if (quality < 4) quality = 4;
        if (quality > 63) quality = 63;
        _jpeg_quality = quality;
        if (!_initialized) return true;
        sensor_t* s = esp_camera_sensor_get();
        if (!s) return false;
        return (s->set_quality(s, quality) == 0);
    }

    /**
     * Flip image vertically (vflip) and/or mirror horizontally (hmirror)
     */
    bool setFlip(bool vflip, bool hmirror = false) {
        if (!_initialized) return false;
        sensor_t* s = esp_camera_sensor_get();
        if (!s) return false;
        int r1 = s->set_vflip(s, vflip ? 1 : 0);
        int r2 = s->set_hmirror(s, hmirror ? 1 : 0);
        return (r1 == 0 && r2 == 0);
    }

    /**
     * Adjust brightness (-2 to +2)
     */
    bool setBrightness(int level) {
        if (!_initialized) return false;
        sensor_t* s = esp_camera_sensor_get();
        return s ? (s->set_brightness(s, constrain(level, -2, 2)) == 0) : false;
    }

    /**
     * Adjust contrast (-2 to +2)
     */
    bool setContrast(int level) {
        if (!_initialized) return false;
        sensor_t* s = esp_camera_sensor_get();
        return s ? (s->set_contrast(s, constrain(level, -2, 2)) == 0) : false;
    }

    /**
     * Adjust color saturation (-2 to +2)
     */
    bool setSaturation(int level) {
        if (!_initialized) return false;
        sensor_t* s = esp_camera_sensor_get();
        return s ? (s->set_saturation(s, constrain(level, -2, 2)) == 0) : false;
    }

    /**
     * Apply hardware special effect:
     * 0 = None, 1 = Negative, 2 = Grayscale, 3 = Red Tint, 4 = Green Tint, 5 = Blue Tint, 6 = Sepia
     */
    bool setSpecialEffect(int effect) {
        if (!_initialized) return false;
        sensor_t* s = esp_camera_sensor_get();
        return s ? (s->set_special_effect(s, constrain(effect, 0, 6)) == 0) : false;
    }

    /**
     * Configure White Balance (AWB) and mode:
     * mode: 0 = Auto, 1 = Sunny, 2 = Cloudy, 3 = Office, 4 = Home
     */
    bool setWhiteBalance(bool enable, int mode = 0) {
        if (!_initialized) return false;
        sensor_t* s = esp_camera_sensor_get();
        if (!s) return false;
        s->set_whitebal(s, enable ? 1 : 0);
        s->set_awb_gain(s, enable ? 1 : 0);
        return (s->set_wb_mode(s, constrain(mode, 0, 4)) == 0);
    }

    /**
     * Control onboard camera Flash LED (0 = OFF, 1..255 = PWM brightness)
     */
    void setFlash(uint8_t intensity) {
        if (_pins.led < 0) return;
        setupLedFlash(_pins.led);
        setCameraFlashDuty(intensity);
    }

    /**
     * Get the detected camera ribbon sensor model name (e.g., "OV2640", "OV3660", "OV5640")
     */
    const char* getSensorName() const {
        if (!_initialized) return "UNINITIALIZED";
        sensor_t* s = esp_camera_sensor_get();
        if (!s) return "NONE";
        switch (s->id.PID) {
            case OV2640_PID: return "OV2640";
            case OV3660_PID: return "OV3660";
            case OV5640_PID: return "OV5640";
            case OV7725_PID: return "OV7725";
            case OV7670_PID: return "OV7670";
            case NT99141_PID: return "NT99141";
            case GC2145_PID: return "GC2145";
            case GC032A_PID: return "GC032A";
            case GC0308_PID: return "GC0308";
            case BF3005_PID: return "BF3005";
            case BF20A6_PID: return "BF20A6";
            case SC030IOT_PID: return "SC030IOT";
            default: return "GENERIC_CAM";
        }
    }

    /**
     * Get the active camera carrier board name
     */
    const char* getBoardName() const {
        return _pins.boardName ? _pins.boardName : "ESP32-CAM";
    }

    /**
     * Direct access to the underlying esp_camera sensor_t pointer for custom register tweaks
     */
    sensor_t* getSensor() {
        return _initialized ? esp_camera_sensor_get() : nullptr;
    }

private:
    bool        _initialized;
    bool        _server_started;
    CameraBoard _board;
    CameraPins  _pins;
    framesize_t _frame_size;
    int         _jpeg_quality;
    int         _xclk_mhz;

    bool _initWithPins(const CameraPins& pins, framesize_t frameSize, int jpegQuality, int xclkMhz) {
        // Power-cycle camera sensor if PWDN pin is wired
        if (pins.pwdn >= 0) {
            pinMode(pins.pwdn, OUTPUT);
            digitalWrite(pins.pwdn, LOW);
            delay(10);
        }

        camera_config_t config = {};
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer   = LEDC_TIMER_0;
        config.pin_d0       = pins.y2;
        config.pin_d1       = pins.y3;
        config.pin_d2       = pins.y4;
        config.pin_d3       = pins.y5;
        config.pin_d4       = pins.y6;
        config.pin_d5       = pins.y7;
        config.pin_d6       = pins.y8;
        config.pin_d7       = pins.y9;
        config.pin_xclk     = pins.xclk;
        config.pin_pclk     = pins.pclk;
        config.pin_vsync    = pins.vsync;
        config.pin_href     = pins.href;
        config.pin_sccb_sda = pins.siod;
        config.pin_sccb_scl = pins.sioc;
        config.pin_pwdn     = pins.pwdn;
        config.pin_reset    = pins.reset;
        config.xclk_freq_hz = (xclkMhz > 0 ? xclkMhz : 20) * 1000000;
        config.frame_size   = frameSize;
        config.pixel_format = PIXFORMAT_JPEG;
        config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
        config.fb_location  = CAMERA_FB_IN_PSRAM;
        config.jpeg_quality = (jpegQuality >= 4 && jpegQuality <= 63) ? jpegQuality : 12;
        config.fb_count     = 1;

        if (config.pin_xclk != -1) {
            if (psramFound()) {
                if (jpegQuality < 4 || jpegQuality > 63) {
                    config.jpeg_quality = 10;
                }
                config.fb_count  = 2;
                config.grab_mode = CAMERA_GRAB_LATEST;
            } else {
                // Without PSRAM, cap frame size to SVGA/QVGA and store frame buffer in internal DRAM
                if (config.frame_size > FRAMESIZE_SVGA) {
                    config.frame_size = FRAMESIZE_QVGA;
                }
                config.fb_location = CAMERA_FB_IN_DRAM;
            }
        }

        INTEGRALL_LOG_INFO_VAL("Camera: Initializing board ", pins.boardName ? pins.boardName : "ESP32-CAM");
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            INTEGRALL_LOG_ERROR("Camera: Hardware init failed! Verify board model and ribbon cable.");
            return false;
        }

        _initialized = true;

        // Sync Flash LED pin with app_httpd
        if (pins.led >= 0) {
            setupLedFlash(pins.led);
        }

        // Apply automatic sensor-specific (OV2640 / OV3660 / OV5640) and board-specific tuning
        sensor_t* s = esp_camera_sensor_get();
        if (s) {
            if (pins.defaultVFlip)   s->set_vflip(s, 1);
            if (pins.defaultHMirror) s->set_hmirror(s, 1);

            if (s->id.PID == OV3660_PID) {
                s->set_vflip(s, 1);       // OV3660 sensors are inverted by default
                s->set_brightness(s, 1);  // Slightly lift brightness
                s->set_saturation(s, -2); // Reduce excessive default saturation
            } else if (s->id.PID == OV5640_PID) {
                s->set_vflip(s, 1);       // OV5640 sensors are inverted by default
            }
            INTEGRALL_LOG_INFO_VAL("Camera: Detected sensor ", getSensorName());
        }

        INTEGRALL_LOG_INFO("Camera: Hardware Ready");
        return true;
    }
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_CAMERA_ENABLED
#endif // INTEGRALL_CAMERA_MODULE_H
