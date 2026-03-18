/**
 * TigoAI Smart Glasses Firmware - Powered by Integrall Framework
 * Target: ESP32-CAM (AI-Thinker)
 */

// 1. Integrall Configuration
#define INTEGRALL_ENABLE_CAMERA
#define INTEGRALL_ENABLE_WIFI
#define INTEGRALL_ENABLE_AUDIO
#define INTEGRALL_DEBUG_LEVEL 3

#include <Integrall.h>
#include <ArduinoJson.h>

// --- Project Configuration ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://YOUR_BACKEND_IP:5000"; 
const char* userId = "user_123";

Integrall::System integrall;

void setup() {
    // 2. Initialize Integrall
    if (!integrall.begin(ssid, password, serverUrl)) {
        Serial.println("Integrall failed to start!");
        return;
    }
    
    // Camera is automatically initialized by Integrall due to EN_CAMERA flag
    integrall.println("Smart Glasses Ready. Device ID: %s", integrall.getDeviceId());
}

/**
 * Capture and upload photo for AI analysis
 */
void captureAndAnalyze() {
    integrall.println("Capturing for analysis...");
    
    camera_fb_t * fb = integrall.cameraCapture();
    if (!fb) {
        integrall.println("Camera capture failed");
        return;
    }

    integrall.println("Uploading image (%d bytes)...", fb->len);
    
    // Using the new multipart helper from Integrall
    String uploadUrl = String(serverUrl) + "/upload_media";
    int httpResponseCode = integrall.httpPostFile(
        uploadUrl.c_str(), 
        fb->buf, 
        fb->len, 
        "capture.jpg", 
        "file", 
        "user_id", 
        userId
    );
    
    if (httpResponseCode > 0) {
        // Parse response for AI analysis result
        StaticJsonDocument<512> doc;
        deserializeJson(doc, integrall.getLastResponse());
        const char* analysis = doc["analysis"];
        if (analysis) {
            integrall.println("AI Description: %s", analysis);
        } else {
            integrall.println("Upload Success, code: %d", httpResponseCode);
        }
    } else {
        integrall.println("Error on uploading image: %d", httpResponseCode);
    }
    
    integrall.cameraRelease(fb);
}

/**
 * Fetch and play the AI response audio
 */
void playResponse(String path) {
    String fullUrl = String(serverUrl) + path;
    integrall.println("Streaming audio from: %s", fullUrl.c_str());
    
    Stream* stream = integrall.getStream(fullUrl.c_str());
    if (stream) {
        // Use the stream to feed an I2S speaker
        // In a real app, you'd use a library like ESP32-audioI2S here
        integrall.println("Streaming data chunk by chunk...");
        
        // Placeholder for real I2S feeding logic
        size_t total = 0;
        while(stream->available()) {
            static uint8_t buf[512];
            size_t n = stream->readBytes(buf, sizeof(buf));
            total += n;
            // i2s_write(..., buf, n, ...);
        }
        
        integrall.println("Stream finished. Total bytes: %d", total);
        integrall.endStream();
    } else {
        integrall.println("Failed to open audio stream");
    }
}

/**
 * Capture audio and send to transcribe (conceptual)
 */
void sendVoiceCommand() {
    integrall.println("Sending voice command...");
    // Conceptual: Record audio and upload via httpPostFile
    // On success: playResponse(audioUrlPath);
}

void loop() {
    // Standard Integrall house-keeping (WiFi reconnects, etc)
    integrall.handle();
    
    // Example triggers (mocked)
    // if (integrall.inputButtonPressed(0)) captureAndAnalyze();
    
    delay(10);
}
