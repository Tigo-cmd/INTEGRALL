#define INTEGRALL_ENABLE_SENSORS
#include <unity.h>
#include "modules/SensorModule.h"
using namespace Integrall;

void setUp() {
    Fake::reset(); Wire = TwoWire(); Util::i2cState() = Util::I2CState();
    for (uint8_t i = 0; i < Detail::PULSE_SLOTS; i++) Detail::pulseSlot(i) = Detail::PulseSlot();
    Fake::advanceMs(1);   // real micros() is never 0 at an edge
}
void tearDown() {}

void test_dew_point_and_heat_index_known_values() {
    SensorModule s;
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 13.9f, s.calculateDewPoint(25.0f, 50.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 40.9f, s.calculateHeatIndex(35.0f, 50.0f));   // NOAA table ~105.6 F
    TEST_ASSERT_TRUE(isnan(s.calculateDewPoint(25.0f, 0.0f)));
    TEST_ASSERT_TRUE(isnan(s.calculateDewPoint(NAN, 50.0f)));
}

void test_altitude_at_sea_level_is_zero() {
    SensorModule s;
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, s.calculateAltitude(1013.25f));
    TEST_ASSERT_FLOAT_WITHIN(15.0f, 988.0f, s.calculateAltitude(900.0f));
    TEST_ASSERT_TRUE(isnan(s.calculateAltitude(0.0f)));
}

void test_thermistor_25C_and_open_circuit() {
    SensorModule s;
    Fake::b().analog[A0] = s.getAdcMax() / 2;        // equal divider => R_ntc = R0 => 25 C
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 25.0f, s.readThermistorC(A0));
    Fake::b().analog[A0] = 0;
    TEST_ASSERT_TRUE(isnan(s.readThermistorC(A0)));
}

void test_missing_i2c_sensors_return_nan_and_back_off() {   // audit SE5/SE7/SE8
    SensorModule s;
    TEST_ASSERT_TRUE(isnan(s.readLux()));
    TEST_ASSERT_TRUE(isnan(s.readPressure()));
    float t, h; TEST_ASSERT_FALSE(s.readSHT3x(0x44, t, h));
    TEST_ASSERT_TRUE(isnan(t));
}

void test_bh1750_configured_once_then_read() {     // audit SE5
    Wire.present = {0x23};
    SensorModule s;
    TEST_ASSERT_TRUE(isnan(s.readLux()));            // starts measurement, not ready yet
    size_t cmds = Wire.written.size();
    Fake::advanceMs(200);
    Wire.replies = {0x01, 0x2C};                     // 300 raw -> 250 lx
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 250.0f, s.readLux());
    Wire.replies = {0x01, 0x2C};
    s.readLux();
    TEST_ASSERT_EQUAL(cmds, Wire.written.size());    // no mode command re-sent (would restart it)
}

void test_co2_pwm_from_interrupt_timing() {        // audit SE2 (no 2.5 s pulseIn)
    SensorModule s;
    TEST_ASSERT_EQUAL(-1, s.readCO2PWM(6));          // attaches interrupt, no data yet
    // MH-Z19: 1004 ms cycle, TH = 2 + ppm/range*1000 ms. 800 ppm on 5000 range => TH = 162 ms.
    for (int cycle = 0; cycle < 2; cycle++) {
        Fake::setPin(6, HIGH); Fake::advanceMs(162);
        Fake::setPin(6, LOW);  Fake::advanceMs(1004 - 162);
    }
    Fake::setPin(6, HIGH);
    TEST_ASSERT_INT_WITHIN(5, 800, s.readCO2PWM(6));
}

void test_flow_meter_counts_pulses() {             // audit SE3
    SensorModule s;
    s.readWaterFlowLPM(7);                           // start window
    for (int i = 0; i < 75; i++) {                   // 75 pulses in 1 s = 75 Hz -> 10 L/min (7.5 factor)
        Fake::setPin(7, HIGH); Fake::advanceUs(6666);
        Fake::setPin(7, LOW);  Fake::advanceUs(6667);
    }
    Fake::advanceMs(1);
    TEST_ASSERT_FLOAT_WITHIN(0.3f, 10.0f, s.readWaterFlowLPM(7));
}

void test_ultrasonic_is_async_and_median_filtered() {   // audit SE10
    SensorModule s;
    TEST_ASSERT_TRUE(isnan(s.readDistanceCM(5, 6)));  // registers; no echo yet
    const unsigned long echoes[] = {1166, 1166, 5000, 1166, 1166};   // 20 cm, with one outlier
    for (unsigned long e : echoes) {
        s.handle();                                    // sends a ping
        Fake::setPin(6, HIGH); Fake::advanceUs(e); Fake::setPin(6, LOW);
        s.handle();                                    // captures the echo
        Fake::advanceMs(60);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 20.0f, s.readDistanceCM(5, 6, 5));
}

void test_hx711_never_waits_for_data_ready() {     // audit SE4
    SensorModule s;
    Fake::setPin(4, HIGH);                             // DOUT high = not ready
    unsigned long t0 = micros();
    TEST_ASSERT_TRUE(isnan(s.readWeightGrams(4, 5)));
    TEST_ASSERT_LESS_THAN(100, micros() - t0);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_dew_point_and_heat_index_known_values);
    RUN_TEST(test_altitude_at_sea_level_is_zero);
    RUN_TEST(test_thermistor_25C_and_open_circuit);
    RUN_TEST(test_missing_i2c_sensors_return_nan_and_back_off);
    RUN_TEST(test_bh1750_configured_once_then_read);
    RUN_TEST(test_co2_pwm_from_interrupt_timing);
    RUN_TEST(test_flow_meter_counts_pulses);
    RUN_TEST(test_ultrasonic_is_async_and_median_filtered);
    RUN_TEST(test_hx711_never_waits_for_data_ready);
    return UNITY_END();
}
