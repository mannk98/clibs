#include "unity.h"
#include "map_range.h"

#include <stdint.h>

void setUp(void) {}
void tearDown(void) {}

/* Full-range int32 inputs must not overflow the intermediate arithmetic
 * (regression: the subtractions must happen in int64, not int32). Traps under
 * -fsanitize=undefined if the operands are subtracted in int32 first. */
static void test_map_no_int32_overflow(void)
{
    TEST_ASSERT_EQUAL_INT32(100, map_range(INT32_MAX, INT32_MIN, INT32_MAX, 0, 100));
    TEST_ASSERT_EQUAL_INT32(0,   map_range(INT32_MIN, INT32_MIN, INT32_MAX, 0, 100));
    /* in/out spanning the full int32 range, midpoint maps to ~midpoint */
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, map_range(1, 0, 1, INT32_MIN, INT32_MAX));
}

static void test_map_basic(void)
{
    TEST_ASSERT_EQUAL_INT32(50,  map_range(512, 0, 1023, 0, 100));
    TEST_ASSERT_EQUAL_INT32(0,   map_range(0, 0, 1023, 0, 100));
    TEST_ASSERT_EQUAL_INT32(100, map_range(1023, 0, 1023, 0, 100));
}

static void test_map_inverted(void)
{
    TEST_ASSERT_EQUAL_INT32(50,  map_range(5, 0, 10, 100, 0));
    TEST_ASSERT_EQUAL_INT32(100, map_range(0, 0, 10, 100, 0));
}

static void test_map_degenerate(void)
{
    TEST_ASSERT_EQUAL_INT32(42, map_range(7, 5, 5, 42, 99));   /* in_min==in_max -> out_min */
}

static void test_clamp(void)
{
    TEST_ASSERT_EQUAL_INT32(100, clamp_i32(150, 0, 100));
    TEST_ASSERT_EQUAL_INT32(0,   clamp_i32(-5, 0, 100));
    TEST_ASSERT_EQUAL_INT32(50,  clamp_i32(50, 0, 100));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_map_basic);
    RUN_TEST(test_map_inverted);
    RUN_TEST(test_map_degenerate);
    RUN_TEST(test_map_no_int32_overflow);
    RUN_TEST(test_clamp);
    return UNITY_END();
}
