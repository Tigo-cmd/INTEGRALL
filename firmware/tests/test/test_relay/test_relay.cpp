#define INTEGRALL_ENABLE_RELAY
#include <unity.h>
#include "modules/RelayModule.h"
using namespace Integrall;

void setUp() { Fake::reset(); }
void tearDown() {}

void test_gpio0_is_a_valid_relay_pin() {          // audit R1
    RelayModule r;
    int i = r.addRelay(0, true, "gpio0");
    TEST_ASSERT_EQUAL(0, i);
    TEST_ASSERT_TRUE(r.isValid(0));
    TEST_ASSERT_TRUE(r.on(0));
    TEST_ASSERT_EQUAL(LOW, digitalRead(0));        // active-low: ON = LOW
}

void test_active_low_and_high_levels() {
    RelayModule r;
    int lo = r.addRelay(5, true);
    int hi = r.addRelay(6, false);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(5));        // starts OFF
    TEST_ASSERT_EQUAL(LOW, digitalRead(6));
    r.on(lo); r.on(hi);
    TEST_ASSERT_EQUAL(LOW, digitalRead(5));
    TEST_ASSERT_EQUAL(HIGH, digitalRead(6));
}

void test_safety_timeout_turns_off_exactly_after_duration() {
    RelayModule r;
    int i = r.addRelay(5);
    r.setSafetyTimeout(i, 1000);
    r.on(i);
    Fake::advanceMs(999); r.handle();
    TEST_ASSERT_TRUE(r.isOn(i));
    Fake::advanceMs(1); r.handle();
    TEST_ASSERT_FALSE(r.isOn(i));
}

void test_interlock_refuses_second_relay_in_group() { // audit R2
    RelayModule r;
    int fwd = r.addRelay(5), rev = r.addRelay(6);
    r.setInterlockGroup(fwd, 1); r.setInterlockGroup(rev, 1);
    TEST_ASSERT_TRUE(r.on(fwd));
    TEST_ASSERT_FALSE(r.on(rev));                   // refused, never both on
    TEST_ASSERT_FALSE(r.isOn(rev));
    r.off(fwd);
    TEST_ASSERT_TRUE(r.on(rev));
}

void test_button_toggles_once_per_debounced_press() {
    RelayModule r;
    int i = r.addRelay(5);
    Fake::setPin(4, HIGH);                          // button idle: pulled up
    r.attachTrigger(i, 4, true);
    r.handle();
    Fake::setPin(4, LOW);  r.handle();              // press (bouncing begins)
    Fake::advanceMs(10); Fake::setPin(4, HIGH); r.handle();
    Fake::advanceMs(5);  Fake::setPin(4, LOW);  r.handle();
    TEST_ASSERT_FALSE(r.isOn(i));                   // not yet: still bouncing
    Fake::advanceMs(60); r.handle();
    TEST_ASSERT_TRUE(r.isOn(i));
    Fake::advanceMs(500); r.handle();
    TEST_ASSERT_TRUE(r.isOn(i));                    // holding doesn't re-toggle
    Fake::setPin(4, HIGH); r.handle(); Fake::advanceMs(60); r.handle();   // release
    Fake::setPin(4, LOW);  r.handle(); Fake::advanceMs(60); r.handle();   // press again
    TEST_ASSERT_FALSE(r.isOn(i));                   // second press toggles off
}

void test_stats_count_beyond_255() {              // audit R3
    RelayModule r;
    int i = r.addRelay(5);
    for (int k = 0; k < 600; k++) r.toggle(i);
    TEST_ASSERT_EQUAL_UINT32(600, r.getToggleCount(i));
}

void test_on_time_accumulates() {
    RelayModule r;
    int i = r.addRelay(5);
    r.on(i); Fake::advanceMs(300); r.off(i);
    Fake::advanceMs(1000);
    r.on(i); Fake::advanceMs(200);
    TEST_ASSERT_EQUAL_UINT32(500, r.getTotalOnTime(i));
}

void test_capacity_reports_full() {
    RelayModule r;
    for (int k = 0; k < INTEGRALL_MAX_RELAYS; k++) TEST_ASSERT_EQUAL(k, r.addRelay(10 + k));
    TEST_ASSERT_EQUAL(-1, r.addRelay(40));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_gpio0_is_a_valid_relay_pin);
    RUN_TEST(test_active_low_and_high_levels);
    RUN_TEST(test_safety_timeout_turns_off_exactly_after_duration);
    RUN_TEST(test_interlock_refuses_second_relay_in_group);
    RUN_TEST(test_button_toggles_once_per_debounced_press);
    RUN_TEST(test_stats_count_beyond_255);
    RUN_TEST(test_on_time_accumulates);
    RUN_TEST(test_capacity_reports_full);
    return UNITY_END();
}
