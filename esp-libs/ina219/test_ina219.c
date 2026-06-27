/* Unity host test for the pure part of the INA219 L3 driver.
 * Tests ONLY the pure API contract in ina219.h (bus/shunt/current/power
 * conversions + calibration). The SDK glue (ina219_dev.{h,c} over i2c_bus)
 * is COMPILE_GATE-only / deferred and not exercised here.
 * The header does not exist yet — this is the RED step of TDD. */
#include "unity.h"
#include "ina219.h"

void setUp(void)    {}
void tearDown(void) {}

/* Acceptance 1: bus voltage register drops the 3 status bits then scales by the
 * 4 mV LSB. raw 0x1F40 == 8000 -> (8000>>3)*4 == 1000*4 == 4000 mV. */
static void test_ina219_bus_mv_0x1F40_is_4000(void)
{
    TEST_ASSERT_EQUAL_INT32(4000, ina219_bus_mv(0x1F40u));
}

/* Acceptance 2: zero bus register -> 0 mV. */
static void test_ina219_bus_mv_zero_is_0(void)
{
    TEST_ASSERT_EQUAL_INT32(0, ina219_bus_mv(0x0000u));
}

/* Acceptance 3: shunt voltage is signed, 10 uV per LSB.
 * raw 1000 -> 1000*10 == 10000 uV. */
static void test_ina219_shunt_uv_pos_1000_is_10000(void)
{
    TEST_ASSERT_EQUAL_INT32(10000, ina219_shunt_uv((int16_t)1000));
}

/* Acceptance 4: negative shunt voltage stays signed.
 * raw -1000 -> -1000*10 == -10000 uV. */
static void test_ina219_shunt_uv_neg_1000_is_neg_10000(void)
{
    TEST_ASSERT_EQUAL_INT32(-10000, ina219_shunt_uv((int16_t)-1000));
}

/* Acceptance 5: current = signed raw * current_lsb (uA).
 * raw 1000, lsb 100 uA -> 100000 uA. */
static void test_ina219_current_ua_pos_is_100000(void)
{
    TEST_ASSERT_EQUAL_INT32(100000, ina219_current_ua((int16_t)1000, 100u));
}

/* Acceptance 6: current stays signed for a negative raw.
 * raw -1000, lsb 100 uA -> -100000 uA. */
static void test_ina219_current_ua_neg_is_neg_100000(void)
{
    TEST_ASSERT_EQUAL_INT32(-100000, ina219_current_ua((int16_t)-1000, 100u));
}

/* Acceptance 7: power = raw * 20 * current_lsb (uW), raw unsigned.
 * raw 1000, lsb 100 uA -> 1000*20*100 == 2000000 uW. */
static void test_ina219_power_uw_is_2000000(void)
{
    TEST_ASSERT_EQUAL_INT32(2000000, ina219_power_uw(1000u, 100u));
}

/* Acceptance 8: calibration register, datasheet example (0.1 ohm shunt, 100 uA
 * LSB). denom = current_lsb_ua * shunt_milliohm = 100*100 = 10000;
 * 40960000 / 10000 == 4096. */
static void test_ina219_calibration_datasheet_is_4096(void)
{
    TEST_ASSERT_EQUAL_UINT16(4096u, ina219_calibration(100u, 100u));
}

/* Acceptance 9: calibration with a zero denominator (shunt 0) returns 0 instead
 * of dividing by zero. */
static void test_ina219_calibration_zero_denom_is_0(void)
{
    TEST_ASSERT_EQUAL_UINT16(0u, ina219_calibration(100u, 0u));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ina219_bus_mv_0x1F40_is_4000);
    RUN_TEST(test_ina219_bus_mv_zero_is_0);
    RUN_TEST(test_ina219_shunt_uv_pos_1000_is_10000);
    RUN_TEST(test_ina219_shunt_uv_neg_1000_is_neg_10000);
    RUN_TEST(test_ina219_current_ua_pos_is_100000);
    RUN_TEST(test_ina219_current_ua_neg_is_neg_100000);
    RUN_TEST(test_ina219_power_uw_is_2000000);
    RUN_TEST(test_ina219_calibration_datasheet_is_4096);
    RUN_TEST(test_ina219_calibration_zero_denom_is_0);
    return UNITY_END();
}
