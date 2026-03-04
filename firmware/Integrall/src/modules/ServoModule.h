/**
 * ServoModule.h
 * 
 * Simple wrapper for Servo motors in the Integrall Framework.
 * On ESP32, this handles PWM timers behind the scenes.
 * On standard Arduinos, it uses the standard Servo library.
 */

#ifndef INTEGRALL_SERVO_MODULE_H
#define INTEGRALL_SERVO_MODULE_H

#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_SERVO_ENABLED

#include <Arduino.h>

#if defined(ESP32)
  #include <ESP32Servo.h> // Recommended for ESP32
#else
  #include <Servo.h>
#endif

namespace Integrall {

class ServoModule {
public:
    ServoModule() : _attached(false) {}

    /**
     * Attach a servo to a specific GPIO pin
     */
    void attach(uint8_t pin) {
        if (_attached) return;
        _servo.attach(pin);
        _attached = true;
    }

    /**
     * Set the servo angle (0 to 180)
     */
    void set(uint8_t angle) {
        if (!_attached) return;
        if (angle > 180) angle = 180;
        _servo.write(angle);
    }

    /**
     * Map an analog pin directly to servo angle (0-180)
     * Useful for potentiometers.
     */
    void setFromAnalog(uint8_t pin) {
        if (!_attached) return;
        int val = analogRead(pin);
        int angle = map(val, 0, 1023, 0, 180);
        set(angle);
    }

    /**
     * Non-blocking sweep (call this in loop)
     * Automatically moves servo back and forth.
     */
    void updateSweep(uint32_t speed_ms = 15) {
        if (!_attached) return;
        static unsigned long last_move = 0;
        static int current_pos = 0;
        static int direction = 1;

        if (millis() - last_move > speed_ms) {
            current_pos += direction;
            if (current_pos >= 180 || current_pos <= 0) direction *= -1;
            set(current_pos);
            last_move = millis();
        }
    }

    bool isAttached() const { return _attached; }

private:
#if defined(ESP32)
    Servo _servo;
#else
    Servo _servo;
#endif
    bool _attached;
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_SERVO_ENABLED
#endif // INTEGRALL_SERVO_MODULE_H
