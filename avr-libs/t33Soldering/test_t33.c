#include "unity.h"
#include "T33Soldering.h"

void setUp(void) {}
void tearDown(void) {}

/* v = adc * 0.00716 (mV); per-band offsets 25/32/23/19. Expected = hand-computed. */
static void test_adc_zero_is_zero(void) {
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, t33_vol_to_temp(0));
}
static void test_band1(void) {   /* adc=300 -> v=2.148 -> 62.98*2.148+7.74+25 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 168.02f, t33_vol_to_temp(300));
}
static void test_band2(void) {   /* adc=500 -> v=3.58 -> 57.07*3.58+23.74+32 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 260.05f, t33_vol_to_temp(500));
}
static void test_band3(void) {   /* adc=700 -> v=5.012 -> 54.47*5.012+34.88+23 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 330.88f, t33_vol_to_temp(700));
}
static void test_band4(void) {   /* adc=900 -> v=6.444 -> 52.77*6.444+44.68+19 */
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 403.73f, t33_vol_to_temp(900));
}
static void test_monotonic_increasing(void) {
    TEST_ASSERT_TRUE(t33_vol_to_temp(900) > t33_vol_to_temp(300));
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
