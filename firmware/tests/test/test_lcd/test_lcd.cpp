#define INTEGRALL_ENABLE_LCD
#include <unity.h>
#include "modules/LCDModule.h"
using namespace Integrall;

void setUp() { Fake::reset(); Wire = TwoWire(); Util::i2cState() = Util::I2CState(); }
void tearDown() {}

static LiquidCrystal_I2C* raw(LCDModule& m) { return m.getRaw(); }

void test_missing_lcd_is_reported() {              // audit L2
    LCDModule lcd;
    TEST_ASSERT_FALSE(lcd.begin());
    TEST_ASSERT_FALSE(lcd.isPresent());
    lcd.print("hello"); lcd.handle();               // must be a safe no-op
}

void test_autodetects_0x3F() {
    Wire.present = {0x3F};
    LCDModule lcd;
    TEST_ASSERT_TRUE(lcd.begin());
    TEST_ASSERT_EQUAL_HEX8(0x3F, lcd.address());
}

void test_label_and_value_on_same_row_both_visible() {   // audit L1
    Wire.present = {0x27};
    LCDModule lcd; lcd.begin();
    lcd.print("Temp:", 0, 0);
    lcd.print("25.1", 10, 0);
    lcd.handle();
    TEST_ASSERT_EQUAL_STRING("Temp:     25.1  ", raw(lcd)->row(0).c_str());
}

void test_col0_print_replaces_line() {
    Wire.present = {0x27};
    LCDModule lcd; lcd.begin();
    lcd.print("Temp: 25", 0, 1); lcd.handle();
    lcd.print("Temp: 9", 0, 1);  lcd.handle();
    TEST_ASSERT_EQUAL_STRING("Temp: 9         ", raw(lcd)->row(1).c_str());   // no stale "5"
}

void test_identical_prints_cost_no_i2c_traffic() {
    Wire.present = {0x27};
    LCDModule lcd; lcd.begin();
    lcd.print("Temp:", 0, 0); lcd.print("25.1", 10, 0); lcd.handle();
    int before = raw(lcd)->charWrites;
    for (int i = 0; i < 100; i++) { lcd.print("Temp:", 0, 0); lcd.print("25.1", 10, 0); lcd.handle(); }
    TEST_ASSERT_EQUAL(before, raw(lcd)->charWrites);
}

void test_long_text_scrolls_without_blocking() {
    Wire.present = {0x27};
    LCDModule lcd; lcd.begin();
    lcd.scrollText("This line is far too long for sixteen", 0, 300);
    lcd.handle();
    std::string first = raw(lcd)->row(0);
    Fake::advanceMs(300); lcd.handle();
    TEST_ASSERT_TRUE(first != raw(lcd)->row(0));
    TEST_ASSERT_EQUAL_STRING("his line is far ", raw(lcd)->row(0).c_str());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_missing_lcd_is_reported);
    RUN_TEST(test_autodetects_0x3F);
    RUN_TEST(test_label_and_value_on_same_row_both_visible);
    RUN_TEST(test_col0_print_replaces_line);
    RUN_TEST(test_identical_prints_cost_no_i2c_traffic);
    RUN_TEST(test_long_text_scrolls_without_blocking);
    return UNITY_END();
}
