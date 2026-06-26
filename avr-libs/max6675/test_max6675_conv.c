#include "unity.h"
#include "max6675_conv.h"

void setUp(void) {}
void tearDown(void) {}

static void test_open_thermocouple(void) {
    /* bit 2 (0x4) set => open => MAX6675_OPEN */
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, MAX6675_OPEN, max6675_raw_to_celsius(0x0004));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, MAX6675_OPEN, max6675_raw_to_celsius(0x0C84));
}
static void test_raw_to_celsius(void) {
    /* raw>>3 then *0.25; 0x0C80 (bit2 clear) -> 400*0.25 = 100.0 */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 100.0f, max6675_raw_to_celsius(0x0C80));
    /* 0x08 -> 1*0.25 = 0.25 */
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.25f, max6675_raw_to_celsius(0x0008));
    /* 0x00 -> 0 */
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.0f, max6675_raw_to_celsius(0x0000));
}
static void test_celsius_to_fahrenheit(void) {
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 32.0f,  max6675_celsius_to_fahrenheit(0.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, 212.0f, max6675_celsius_to_fahrenheit(100.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, -40.0f, max6675_celsius_to_fahrenheit(-40.0f));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_open_thermocouple);
    RUN_TEST(test_raw_to_celsius);
    RUN_TEST(test_celsius_to_fahrenheit);
    return UNITY_END();
}
