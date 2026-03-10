/**
 * StepperModule.h
 * 
 * Simplified control for 28BYJ-48 or generic A4988/DRV8825 stepper drivers.
 * Supports basic steps, RPM-based speed, and target degrees.
 */

#ifndef INTEGRALL_STEPPER_MODULE_H
#define INTEGRALL_STEPPER_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

// Optional: include Stepper support if user has the library installed
#include <Stepper.h>

namespace Integrall {

class StepperModule {
public:
    StepperModule() : _stepsPerRev(2048), _rpm(15), _pStepper(nullptr) {}

    /**
     * Initialize stepper motor (4-wire example like ULN2003)
     */
    void begin(int stepsPerRev, int p1, int p2, int p3, int p4) {
        _stepsPerRev = stepsPerRev;
        _pStepper = new Stepper(_stepsPerRev, p1, p2, p3, p4);
        _pStepper->setSpeed(_rpm);
    }

    /**
     * Set motor speed (RPM)
     */
    void setSpeed(int rpm) {
        _rpm = rpm;
        if (_pStepper) _pStepper->setSpeed(_rpm);
    }

    /**
     * Move N steps (Blocking - use with caution in IoT loops)
     */
    void step(int steps) {
        if (_pStepper) _pStepper->step(steps);
    }

    /**
     * Move to a specific degree (relative to start)
     */
    void moveDegrees(int deg) {
        if (!_pStepper) return;
        int steps = map(deg, 0, 360, 0, _stepsPerRev);
        _pStepper->step(steps);
    }

private:
    int _stepsPerRev;
    int _rpm;
    Stepper* _pStepper;
};

} // namespace Integrall

#endif // INTEGRALL_STEPPER_MODULE_H
