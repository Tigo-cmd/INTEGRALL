/**
 * SmartFallDevice.ino
 *
 * Wearable fall detector: MPU6050 + buzzer + SIM800L, fully non-blocking.
 * The ESP32 creates its own WiFi hotspot ("Integrall_Fall_Device", password
 * "fallsafe123") with a dashboard at http://192.168.4.1 to set the emergency
 * phone number.
 *
 * Flow (a state machine — loop() never stalls):
 *   MONITORING --impact + horizontal--> CONFIRMING (2 s: did they get up?)
 *   CONFIRMING --still horizontal-->     ALARM (buzzer, 10 s to cancel)
 *   ALARM      --hold button 3 s-->      MONITORING (cancelled)
 *   ALARM      --10 s elapsed-->         SMS queued -> COOLDOWN (30 s) -> MONITORING
 *
 * Wiring: MPU6050 SDA 21 / SCL 22, buzzer 27, cancel button 26 -> GND,
 *         SIM800L TX->16, RX->17 (separate 4 V / 2 A supply!).
 */

#define INTEGRALL_ENABLE_WIFI
#define INTEGRALL_ENABLE_CELLULAR
#define INTEGRALL_ENABLE_IMU
#define INTEGRALL_ENABLE_BUZZER
#define INTEGRALL_ENABLE_INPUT
#define INTEGRALL_DEBUG_LEVEL 3
#include <Integrall.h>

Integrall::System integrall;

#include "Dashboard.h"

const uint8_t BUZZER_PIN = 27;
const uint8_t CANCEL_BTN = 26;

// Impact threshold in m/s^2 (1 g = 9.81). ~2.5 g catches real falls, not walking.
const float IMPACT_THRESHOLD = 25.0f;

enum class FallState { MONITORING, CONFIRMING, ALARM, COOLDOWN };
FallState state = FallState::MONITORING;
unsigned long stateSince = 0;
unsigned long holdStart = 0;

void enter(FallState s) { state = s; stateSince = millis(); }

void onSmsResult(bool ok) {
    integrall.println(ok ? "SMS delivered to network" : "SMS FAILED - check SIM/credit/signal");
    lastSmsStatus = ok ? "sent" : "failed";
}

void setup() {
    integrall.beginAP("Integrall_Fall_Device", "fallsafe123");

    if (!integrall.enableIMU(21, 22)) integrall.println("MPU6050 missing - fall detection disabled");
    integrall.enableBuzzer(BUZZER_PIN);
    integrall.cellularBegin(16, 17);        // returns immediately; modem initialises in background
    integrall.onSMSResult(onSmsResult);

    setupDashboard();
    integrall.selfTest();
}

void loop() {
    integrall.handle();                     // WiFi AP, web dashboard, modem AT engine, buzzer...
    const unsigned long now = millis();

    switch (state) {
        case FallState::MONITORING:
            if (integrall.getImpactForce() > IMPACT_THRESHOLD && integrall.isHorizontal()) {
                INTEGRALL_LOG_INFO("Impact detected, confirming...");
                enter(FallState::CONFIRMING);
            }
            break;

        case FallState::CONFIRMING:          // give them 2 s to get back up
            if (now - stateSince >= 2000) {
                if (integrall.isHorizontal()) {
                    INTEGRALL_LOG_INFO("FALL CONFIRMED");
                    enter(FallState::ALARM);
                } else {
                    enter(FallState::MONITORING);
                }
            }
            break;

        case FallState::ALARM:
            if (!integrall.buzzerIsActive()) integrall.buzzerAlert();   // keep sounding
            // Cancel by holding the button for 3 s
            if (integrall.inputButtonHeld(CANCEL_BTN)) {
                if (holdStart == 0) holdStart = now;
                if (now - holdStart >= 3000) {
                    INTEGRALL_LOG_INFO("Alarm cancelled by user");
                    integrall.buzzerOff();
                    holdStart = 0;
                    enter(FallState::MONITORING);
                    break;
                }
            } else {
                holdStart = 0;
            }
            if (now - stateSince >= 10000) {
                integrall.buzzerOff();
                if (!integrall.sendSMS(emergencyNumber.c_str(), "ALERT! Fall detected. Please check on the user.")) {
                    integrall.println("SMS could not be queued");
                    lastSmsStatus = "not queued";
                } else {
                    lastSmsStatus = "sending...";
                }
                enter(FallState::COOLDOWN);
            }
            break;

        case FallState::COOLDOWN:            // avoid re-triggering while they are still down
            if (now - stateSince >= 30000) enter(FallState::MONITORING);
            break;
    }
}
