#include "unity.h"
#include "wallclock_tick.h"

void setUp(void) {}
void tearDown(void) {}

/* Compiled with -DF_CPU=16000000UL: MILLIS_INC=1, FRACT_INC=3, FRACT_MAX=125. */
static void test_one_tick(void) {
    wallclock_accum a = { 0, 0 };
    wallclock_tick(&a);
    TEST_ASSERT_EQUAL_UINT32(1, a.millis);
    TEST_ASSERT_EQUAL_UINT8(3, a.fract);
}
static void test_thousand_ticks_is_1024ms(void) {
    wallclock_accum a = { 0, 0 };
    for (int i = 0; i < 1000; i++) wallclock_tick(&a);
    TEST_ASSERT_EQUAL_UINT32(1024, a.millis); /* 1000 * 1.024 ms */
    TEST_ASSERT_EQUAL_UINT8(0, a.fract);      /* 1000*3 = 3000 = 24*125 exactly */
}
static void test_monotonic(void) {
    wallclock_accum a = { 0, 0 };
    uint32_t prev = 0;
    for (int i = 0; i < 50; i++) {
        wallclock_tick(&a);
        TEST_ASSERT_TRUE(a.millis > prev);
        prev = a.millis;
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_one_tick);
    RUN_TEST(test_thousand_ticks_is_1024ms);
    RUN_TEST(test_monotonic);
    return UNITY_END();
}
