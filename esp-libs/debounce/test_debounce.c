#include "unity.h"
#include "debounce.h"

void setUp(void) {}
void tearDown(void) {}

static void test_rising_after_threshold(void) {
    debounce d;
    debounce_init(&d, 3, 0);
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE,   debounce_update(&d, 1));
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE,   debounce_update(&d, 1));
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_RISING, debounce_update(&d, 1));
    TEST_ASSERT_EQUAL_UINT8(1, debounce_level(&d));
}
static void test_falling_after_threshold(void) {
    debounce d;
    debounce_init(&d, 2, 1);
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE,    debounce_update(&d, 0));
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_FALLING, debounce_update(&d, 0));
    TEST_ASSERT_EQUAL_UINT8(0, debounce_level(&d));
}
static void test_bounce_ignored(void) {
    debounce d;
    debounce_init(&d, 3, 0);
    debounce_update(&d, 1);                                       /* count 1 */
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE, debounce_update(&d, 0)); /* back to stable */
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE, debounce_update(&d, 1)); /* count 1 again */
    TEST_ASSERT_EQUAL_UINT8(0, debounce_level(&d));               /* never committed */
}
static void test_threshold_zero_becomes_one(void) {
    debounce d;
    debounce_init(&d, 0, 0);
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_RISING, debounce_update(&d, 1)); /* threshold clamped to 1 */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rising_after_threshold);
    RUN_TEST(test_falling_after_threshold);
    RUN_TEST(test_bounce_ignored);
    RUN_TEST(test_threshold_zero_becomes_one);
    return UNITY_END();
}
