/**
 * StepperModule.h
 * 
 * Simplified control for 28BYJ-48 or generic 4-wire stepper drivers.
 * Supports basic steps, RPM-based speed, and target degrees.
 */

#ifndef INTEGRALL_STEPPER_MODULE_H
#define INTEGRALL_STEPPER_MODULE_H

#include <Arduino.h>
#include "../config/IntegrallConfig.h"

#if INTEGRALL_MODULE_STEPPER_ENABLED

#if __has_include(<Stepper.h>)
  #include <Stepper.h>
  #define INTEGRALL_STEPPER_AVAILABLE 1
#else
  #define INTEGRALL_STEPPER_AVAILABLE 0
#endif

namespace Integrall {

class StepperModule {
public:
    StepperModule() : _stepsPerRev(2048), _rpm(15)
    #if INTEGRALL_STEPPER_AVAILABLE
    , _pStepper(nullptr)
    #endif
    {}

    ~StepperModule() {
        #if INTEGRALL_STEPPER_AVAILABLE
        if (_pStepper) {
            delete _pStepper;
            _pStepper = nullptr;
        }
        #endif
    }

    /**
     * Initialize stepper motor (4-wire example like ULN2003)
     */
    void begin(int stepsPerRev, int p1, int p2, int p3, int p4) {
        _stepsPerRev = (stepsPerRev > 0) ? stepsPerRev : 2048;
        #if INTEGRALL_STEPPER_AVAILABLE
        if (_pStepper) {
            delete _pStepper;
        }
        _pStepper = new Stepper(_stepsPerRev, p1, p2, p3, p4);
        _pStepper->setSpeed(_rpm);
        #else
        (void)p1; (void)p2; (void)p3; (void)p4;
        #endif
    }

    /**
     * Set motor speed (RPM)
     */
    void setSpeed(int rpm) {
        if (rpm <= 0) return;
        _rpm = rpm;
        #if INTEGRALL_STEPPER_AVAILABLE
        if (_pStepper) _pStepper->setSpeed(_rpm);
        #endif
    }

    /**
     * Move N steps (Blocking - use with caution in IoT loops)
     */
    void step(int steps) {
        #if INTEGRALL_STEPPER_AVAILABLE
        if (_pStepper) _pStepper->step(steps);
        #else
        (void)steps;
        #endif
    }

    /**
     * Move by a specific angle in degrees
     */
    void moveDegrees(int deg) {
        #if INTEGRALL_STEPPER_AVAILABLE
        if (!_pStepper) return;
        long steps = ((long)deg * (long)_stepsPerRev) / 360L;
        _pStepper->step((int)steps);
        #else
        (void)deg;
        #endif
    }

private:
    int _stepsPerRev;
    int _rpm;
#if INTEGRALL_STEPPER_AVAILABLE
    Stepper* _pStepper;
#endif
};

} // namespace Integrall

#endif // INTEGRALL_MODULE_STEPPER_ENABLED
#endif // INTEGRALL_STEPPER_MODULE_H
