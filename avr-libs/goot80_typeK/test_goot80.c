#include "unity.h"
#include "typeK.h"

void setUp(void) {}
void tearDown(void) {}

/* Expected values re-derived from the documented curve so the test pins the
 * formula independently. v = adc * 0.02237 (mV). */
static void test_adc_zero_is_zero(void) {
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, goot80_vol_to_temp(0));
}
static void test_band1(void) {            /* adc=100 -> v=2.237 -> 24.95*2.237-3.12 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 52.69f, goot80_vol_to_temp(100));
}
static void test_band2(void) {            /* adc=400 -> v=8.948 -> 24.7*8.948-0.97 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 220.05f, goot80_vol_to_temp(400));
}
static void test_band3(void) {            /* adc=550 -> v=12.3035 -> 24.1*12.3035+9.67 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 306.18f, goot80_vol_to_temp(550));
}
static void test_band4(void) {            /* adc=700 -> v=15.659 -> 23.8*15.659+44.68 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 417.36f, goot80_vol_to_temp(700));
}
static void test_monotonic_increasing(void) {
    TEST_ASSERT_TRUE(goot80_vol_to_temp(700) > goot80_vol_to_temp(100));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_adc_zero_is_zero);
    RUN_TEST(test_band1);
    RUN_TEST(test_band2);
    RUN_TEST(test_band3);
    RUN_TEST(test_band4);
    RUN_TEST(test_monotonic_increasing);
    return UNITY_END();
}
