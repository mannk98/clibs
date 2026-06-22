#include "unity.h"
#include "backoff.h"

void setUp(void) {}
void tearDown(void) {}

static void test_sequence_doubles_and_caps(void) {
    backoff b;
    backoff_init(&b, 100, 1000);
    TEST_ASSERT_EQUAL_UINT32(100,  backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(200,  backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(400,  backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(800,  backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(1000, backoff_next(&b)); /* min(1600,1000) */
    TEST_ASSERT_EQUAL_UINT32(1000, backoff_next(&b)); /* stays capped */
}
static void test_reset(void) {
    backoff b;
    backoff_init(&b, 100, 1000);
    backoff_next(&b); backoff_next(&b);
    backoff_reset(&b);
    TEST_ASSERT_EQUAL_UINT32(100, backoff_next(&b));
}
static void test_max_below_base_clamps(void) {
    backoff b;
    backoff_init(&b, 500, 100); /* max < base -> max becomes 500 */
    TEST_ASSERT_EQUAL_UINT32(500, backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(500, backoff_next(&b));
}
static void test_null_guard(void) {
    TEST_ASSERT_EQUAL_UINT32(0, backoff_next(NULL));
}
static void test_base_zero_clamped(void) {
    backoff b;
    backoff_init(&b, 0, 1000);                       /* base 0 -> clamped to 1 */
    TEST_ASSERT_EQUAL_UINT32(1, backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(2, backoff_next(&b));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sequence_doubles_and_caps);
    RUN_TEST(test_reset);
    RUN_TEST(test_max_below_base_clamps);
    RUN_TEST(test_null_guard);
    RUN_TEST(test_base_zero_clamped);
    return UNITY_END();
}
