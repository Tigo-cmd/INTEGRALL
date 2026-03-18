#ifndef INTEGRALL_CAMERA_MODULE_H
#define INTEGRALL_CAMERA_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_CAMERA_ENABLED
#include "esp_camera.h"
#include <WiFi.h>
#include "camera/camera_pins.h"

// External declarations for the server logic in app_httpd.cpp
void startCameraServer();
void setupLedFlash();

#include "../core/Logger.h"

namespace Integrall {

class CameraModule {
public:
    CameraModule() : _initialized(false), _server_started(false) {}

    /**
     * Start the camera hardware with settings optimized for the current board
     */
    bool begin() {
        if (_initialized) return true;

        camera_config_t config;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer = LEDC_TIMER_0;
        config.pin_d0 = Y2_GPIO_NUM;
        config.pin_d1 = Y3_GPIO_NUM;
        config.pin_d2 = Y4_GPIO_NUM;
        config.pin_d3 = Y5_GPIO_NUM;
        config.pin_d4 = Y6_GPIO_NUM;
        config.pin_d5 = Y7_GPIO_NUM;
        config.pin_d6 = Y8_GPIO_NUM;
        config.pin_d7 = Y9_GPIO_NUM;
        config.pin_xclk = XCLK_GPIO_NUM;
        config.pin_pclk = PCLK_GPIO_NUM;
        config.pin_vsync = VSYNC_GPIO_NUM;
        config.pin_href = HREF_GPIO_NUM;
        config.pin_sccb_sda = SIOD_GPIO_NUM;
        config.pin_sccb_scl = SIOC_GPIO_NUM;
        config.pin_pwdn = PWDN_GPIO_NUM;
        config.pin_reset = RESET_GPIO_NUM;
        config.xclk_freq_hz = 20000000;
        config.frame_size = FRAMESIZE_VGA; // Use VGA (640x480) for better speed/lag balance
        config.pixel_format = PIXFORMAT_JPEG; // for streaming
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.jpeg_quality = 12;
        config.fb_count = 1;

        // if PSRAM IC present, allow higher quality but keep VGA as baseline
        if (config.pin_xclk != -1) {
            if (psramFound()) {
                config.jpeg_quality = 10;
                config.fb_count = 2;
                config.grab_mode = CAMERA_GRAB_LATEST;
            } else {
                // Limit the frame size when PSRAM is not available
                config.frame_size = FRAMESIZE_VGA;
                config.fb_location = CAMERA_FB_IN_DRAM;
            }
        }

        // camera init
        INTEGRALL_LOG_INFO("Camera: Initializing hardware...");
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            INTEGRALL_LOG_ERROR("Camera: Hardware init failed!");
            return false;
        }

        INTEGRALL_LOG_INFO("Camera: Hardware Ready");
        _initialized = true;
        
        // Auto-start web server if networking is up
        if (WiFi.status() == WL_CONNECTED) {
            INTEGRALL_LOG_INFO("Camera: Starting MJPEG server...");
            startServer();
        }
        
        return true;
    }

    /**
     * Start the MJPEG stream server
     */
    void startServer() {
        if (!_initialized || _server_started) return;
        
        setupLedFlash();
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

private:
    bool _initialized;
    bool _server_started;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_CAMERA_ENABLED
#endif // INTEGRALL_CAMERA_MODULE_H
