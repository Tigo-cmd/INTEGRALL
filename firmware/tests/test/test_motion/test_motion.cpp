#define INTEGRALL_ENABLE_STEPPER
#define INTEGRALL_ENABLE_BUZZER
#include <unity.h>
#include "modules/StepperModule.h"
#include "modules/BuzzerModule.h"
using namespace Integrall;

void setUp() { Fake::reset(); Fake::advanceMs(1); }
void tearDown() {}

void test_stepper_step_returns_immediately_and_moves_in_background() {   // audit ST1
    StepperModule m;
    m.begin(2048, 8, 9, 10, 11);
    m.setSpeed(15);                                   // 60e6 / 2048 / 15 = 1953 us per step
    unsigned long t0 = micros();
    m.step(10);
    TEST_ASSERT_EQUAL_UINT32(t0, micros());           // no time passed inside step()
    TEST_ASSERT_TRUE(m.isMoving());
    int calls = 0;
    while (m.isMoving() && calls < 1000) { Fake::advanceUs(200); m.handle(); calls++; }
    TEST_ASSERT_EQUAL(10, m.position());
    // first step immediately, then 9 intervals of 1953 us (polled every 200 us)
    TEST_ASSERT_UINT32_WITHIN(600, 9 * 1953 + 300, micros() - t0);
}

void test_stepper_4wire_sequence_matches_arduino_stepper() {
    StepperModule m;
    m.begin(200, 8, 9, 10, 11);
    m.setSpeed(60);
    m.step(1);
    Fake::advanceMs(10); m.handle();
    // phase 1 = 0b0110 -> pins 8..11 = 0,1,1,0
    TEST_ASSERT_EQUAL(0, digitalRead(8)); TEST_ASSERT_EQUAL(1, digitalRead(9));
    TEST_ASSERT_EQUAL(1, digitalRead(10)); TEST_ASSERT_EQUAL(0, digitalRead(11));
    m.step(-1);
    Fake::advanceMs(10); m.handle();
    TEST_ASSERT_EQUAL(0, m.position());
    TEST_ASSERT_EQUAL(1, digitalRead(8)); TEST_ASSERT_EQUAL(0, digitalRead(9));   // back to 0b1010
}

void test_stepper_stop_and_moveTo() {
    StepperModule m;
    m.beginDriver(3, 4, 5, 200);
    m.setSpeed(60);
    m.moveTo(100);
    for (int i = 0; i < 20; i++) { Fake::advanceMs(5); m.handle(); }
    m.stop();
    long p = m.position();
    for (int i = 0; i < 20; i++) { Fake::advanceMs(5); m.handle(); }
    TEST_ASSERT_EQUAL(p, m.position());
    TEST_ASSERT_FALSE(m.isMoving());
}

void test_buzzer_pattern_is_non_blocking() {
    BuzzerModule b;
    b.begin(13);
    b.pattern(3, 100, 50, 2000);
    TEST_ASSERT_EQUAL(2000, Fake::b().toneFreq[13]);
    int beeps = 0; bool on = true;
    for (int t = 0; t < 600; t += 5) {
        Fake::advanceMs(5); b.handle();
        bool now = Fake::b().toneFreq[13] != 0;
        if (now && !on) beeps++;
        on = now;
    }
    TEST_ASSERT_EQUAL(2, beeps);                      // 1st started immediately + 2 more
    TEST_ASSERT_FALSE(b.isActive());
}

void test_buzzer_on_latches_until_off() {
    BuzzerModule b;
    b.begin(25);
    b.on(1800);
    for (int i = 0; i < 50; i++) { Fake::advanceMs(100); b.handle(); }   // 5 s later: still sounding
    TEST_ASSERT_TRUE(b.isOn());
    TEST_ASSERT_TRUE(b.isActive());
    TEST_ASSERT_EQUAL(1800, Fake::b().toneFreq[25]);
    b.off();
    TEST_ASSERT_FALSE(b.isOn());
    TEST_ASSERT_EQUAL(0, Fake::b().toneFreq[25]);
}

void test_active_buzzer_uses_plain_high_low() {
    BuzzerModule b;
    b.begin(26, true);
    b.on();
    TEST_ASSERT_EQUAL(HIGH, digitalRead(26));
    TEST_ASSERT_EQUAL(0, Fake::b().toneFreq[26]);          // never tone() on an active buzzer
    b.off();
    TEST_ASSERT_EQUAL(LOW, digitalRead(26));
    b.beep(100);                                            // patterns work too
    TEST_ASSERT_EQUAL(HIGH, digitalRead(26));
    Fake::advanceMs(120); b.handle();
    TEST_ASSERT_EQUAL(LOW, digitalRead(26));
    TEST_ASSERT_FALSE(b.isActive());
}

void test_beep_cancels_latched_on() {
    BuzzerModule b;
    b.begin(27, true);
    b.on();
    b.beep(50);
    TEST_ASSERT_FALSE(b.isOn());
    Fake::advanceMs(60); b.handle();
    TEST_ASSERT_EQUAL(LOW, digitalRead(27));                // silent after the beep, not stuck on
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_stepper_step_returns_immediately_and_moves_in_background);
    RUN_TEST(test_stepper_4wire_sequence_matches_arduino_stepper);
    RUN_TEST(test_stepper_stop_and_moveTo);
    RUN_TEST(test_buzzer_pattern_is_non_blocking);
    RUN_TEST(test_buzzer_on_latches_until_off);
    RUN_TEST(test_active_buzzer_uses_plain_high_low);
    RUN_TEST(test_beep_cancels_latched_on);
    return UNITY_END();
}
