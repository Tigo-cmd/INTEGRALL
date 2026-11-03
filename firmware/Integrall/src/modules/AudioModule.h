/**
 * AudioModule.h
 * 
 * Logic for voice and high-fidelity sound. 
 * Supports DFPlayer Mini (UART) for MP3 playback and analog/I2S Microphones 
 * for sound recognition/triggering.
 */

#ifndef INTEGRALL_AUDIO_MODULE_H
#define INTEGRALL_AUDIO_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_AUDIO_ENABLED

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
     * @param port   HardwareSerial instance
     * @param rxPin  MCU RX connected to DFPlayer TX
     * @param txPin  MCU TX connected to DFPlayer RX
     */
#if INTEGRALL_DFPLAYER_AVAILABLE
    bool beginDFPlayer(HardwareSerial& port, int rx = -1, int tx = -1) {
        #if defined(ESP32)
        if (rx >= 0 && tx >= 0) {
            port.begin(9600, SERIAL_8N1, rx, tx);
        } else {
            port.begin(9600);
        }
        #else
        (void)rx; (void)tx;
        port.begin(9600);
        #endif
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
     * Get normalized sound level (0-100) from microphone analog input
     */
    int readSoundLevel(uint8_t pin) {
        int val = analogRead(pin);
        #if defined(ESP32)
        return constrain(map(val, 0, 4095, 0, 100), 0, 100);
        #else
        return constrain(map(val, 0, 1023, 0, 100), 0, 100);
        #endif
    }

private:
#if INTEGRALL_DFPLAYER_AVAILABLE
    DFRobotDFPlayerMini _dfPlayer;
#endif
    bool _dfStarted;
    int _volume;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_AUDIO_ENABLED
#endif // INTEGRALL_AUDIO_MODULE_H
