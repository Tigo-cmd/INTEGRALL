#include <unity.h>
#include "config/IntegrallConfig.h"
#include "core/Util.h"
#include "modules/PowerModule.h"
using namespace Integrall;

void setUp() { Fake::reset(); Wire = TwoWire(); Util::i2cState() = Util::I2CState(); Fake::advanceMs(1); }
void tearDown() {}

void test_fmtFloat() {
    char b[16];
    TEST_ASSERT_EQUAL_STRING("24.5", Util::fmtFloat(b, sizeof b, 24.53f, 1));
    TEST_ASSERT_EQUAL_STRING("-3", Util::fmtFloat(b, sizeof b, -3.2f, 0));
    TEST_ASSERT_EQUAL_STRING("nan", Util::fmtFloat(b, sizeof b, NAN, 1));
}

void test_valid_helper() {
    TEST_ASSERT_TRUE(valid(1.0f));
    TEST_ASSERT_FALSE(valid(NAN));
    TEST_ASSERT_FALSE(valid(INFINITY));
}

void test_i2c_present() {
    Wire.present = {0x68};
    TEST_ASSERT_TRUE(Util::i2cPresent(0x68));
    TEST_ASSERT_FALSE(Util::i2cPresent(0x76));
}

void test_backoff_doubles_and_caps() {
    Util::Backoff b;
    TEST_ASSERT_TRUE(b.ready());
    b.fail();                                   // 1 s
    TEST_ASSERT_FALSE(b.ready());
    Fake::advanceMs(1000); TEST_ASSERT_TRUE(b.ready());
    for (int i = 0; i < 10; i++) b.fail();
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(60000, b.step_ms);
}

void test_battery_curve_monotonic() {
    PowerModule p;
    TEST_ASSERT_EQUAL(0, p.getBatteryPercent(3.0f));
    TEST_ASSERT_EQUAL(100, p.getBatteryPercent(4.25f));
    TEST_ASSERT_EQUAL(-1, p.getBatteryPercent(NAN));
    int last = -1;
    for (float v = 3.0f; v <= 4.2f; v += 0.01f) {
        int pct = p.getBatteryPercent(v);
        TEST_ASSERT_GREATER_OR_EQUAL(last, pct);
        last = pct;
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_fmtFloat);
    RUN_TEST(test_valid_helper);
    RUN_TEST(test_i2c_present);
    RUN_TEST(test_backoff_doubles_and_caps);
    RUN_TEST(test_battery_curve_monotonic);
    return UNITY_END();
}
