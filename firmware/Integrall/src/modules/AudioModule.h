/**
 * AudioModule.h
 * 
 * Logic for voice and high-fidelity sound. 
 * Supports DFPlayer Mini (UART) for MP3 playback and I2S Microphones 
 * for sound recognition/triggering. 
 * Replaces simple Buzzers with rich audio output.
 */

#ifndef INTEGRALL_AUDIO_MODULE_H
#define INTEGRALL_AUDIO_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

// Optional: include DFPlayer support if user has the library installed
// Install via Library Manager: "DFRobotDFPlayerMini"
#if __has_include(<DFRobotDFPlayerMini.h>)
  #include <DFRobotDFPlayerMini.h>
  #define INTEGRALL_DFPLAYER_AVAILABLE 1
#else
  #define INTEGRALL_DFPLAYER_AVAILABLE 0
#endif

namespace Integrall {

class AudioModule {
public:
    AudioModule() : _dfStarted(false), _volume(20) {}

    /**
     * Start the DFPlayer Mini via UART
     * @param rxPin  Arduino RX connected to DFPlayer TX
     * @param txPin  Arduino TX connected to DFPlayer RX
     */
#if INTEGRALL_DFPLAYER_AVAILABLE
    bool beginDFPlayer(HardwareSerial& port, int rx, int tx) {
        port.begin(9600, SERIAL_8N1, rx, tx);
        if (!_dfPlayer.begin(port)) return false;
        _dfPlayer.volume(_volume);
        _dfStarted = true;
        return true;
    }

    /**
     * Play specific track from SD card
     * @param trackNumber File ID starting with 0001
     */
    void playTrack(int trackNumber) {
        if (!_dfStarted) return;
        _dfPlayer.play(trackNumber);
    }

    /**
     * Set playback volume (0-30)
     */
    void setVolume(int v) {
        _volume = constrain(v, 0, 30);
        if (_dfStarted) _dfPlayer.volume(_volume);
    }
#endif

    /**
     * Get decibel-like value from I2S Microphone (normalized logic)
     * For ESP32 internal sound monitoring.
     */
#if defined(ESP32)
    int readSoundLevel(uint8_t pin) {
        // Implementation for I2S would require driver/i2s.h
        // Here we provide a simplified analog fallback for generic mics
        int val = analogRead(pin);
        return map(val, 0, 4095, 0, 100);
    }
#endif

private:
#if INTEGRALL_DFPLAYER_AVAILABLE
    DFRobotDFPlayerMini _dfPlayer;
#endif
    bool _dfStarted;
    int _volume;
};

} // namespace Integrall

#endif // INTEGRALL_AUDIO_MODULE_H
