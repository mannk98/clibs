/* RED test for the saturating_counter L1 lib.
 * Tests ONLY the API contract; the header/impl do not exist yet.
 * One test per acceptance bullet. No malloc; stack storage only. */
#include "unity.h"
#include "saturating_counter.h"   /* header does not exist yet -> RED */

#include <stdint.h>

void setUp(void) {}
void tearDown(void) {}

/* Bullet 1: init within range stores value+bounds; get returns value. */
static void test_saturating_counter_init_stores_value_and_bounds(void) {
    saturating_counter c;
    saturating_counter_init(&c, 0, 10, 5);
    TEST_ASSERT_EQUAL_INT32(5,  saturating_counter_get(&c));
    TEST_ASSERT_EQUAL_INT32(0,  c.min);
    TEST_ASSERT_EQUAL_INT32(10, c.max);
}

/* Bullet 2: initial below min is clamped up to min. */
static void test_saturating_counter_init_clamps_below_min(void) {
    saturating_counter c;
    saturating_counter_init(&c, 0, 10, -3);
    TEST_ASSERT_EQUAL_INT32(0, saturating_counter_get(&c));
}

/* Bullet 3: initial above max is clamped down to max. */
static void test_saturating_counter_init_clamps_above_max(void) {
    saturating_counter c;
    saturating_counter_init(&c, 0, 10, 99);
    TEST_ASSERT_EQUAL_INT32(10, saturating_counter_get(&c));
}

/* Bullet 4: inverted bounds (min>max) are swapped so min<=max; value clamps in. */
static void test_saturating_counter_init_swaps_inverted_bounds(void) {
    saturating_counter c;
    saturating_counter_init(&c, 10, 0, 5);
    TEST_ASSERT_EQUAL_INT32(0,  c.min);
    TEST_ASSERT_EQUAL_INT32(10, c.max);
    TEST_ASSERT_EQUAL_INT32(5,  saturating_counter_get(&c));
}

/* Bullet 5: degenerate range min==max==7 pins value; inc and dec stay at 7. */
static void test_saturating_counter_degenerate_range_pins_value(void) {
    saturating_counter c;
    saturating_counter_init(&c, 7, 7, 123); /* any initial -> 7 */
    TEST_ASSERT_EQUAL_INT32(7, saturating_counter_get(&c));
    TEST_ASSERT_EQUAL_INT32(7, saturating_counter_inc(&c));
    TEST_ASSERT_EQUAL_INT32(7, saturating_counter_dec(&c));
    TEST_ASSERT_EQUAL_INT32(7, saturating_counter_get(&c));
}

/* Bullet 6: inc inside range returns +1 and is observable via get. */
static void test_saturating_counter_inc_inside_range(void) {
    saturating_counter c;
    saturating_counter_init(&c, 0, 10, 5);
    TEST_ASSERT_EQUAL_INT32(6, saturating_counter_inc(&c));
    TEST_ASSERT_EQUAL_INT32(6, saturating_counter_get(&c));
}

/* Bullet 7: inc at max saturates (no wrap) and stays at max across repeats. */
static void test_saturating_counter_inc_saturates_at_max(void) {
    saturating_counter c;
    saturating_counter_init(&c, 0, 10, 10);
    TEST_ASSERT_EQUAL_INT32(10, saturating_counter_inc(&c));
    TEST_ASSERT_EQUAL_INT32(10, saturating_counter_inc(&c));
    TEST_ASSERT_EQUAL_INT32(10, saturating_counter_get(&c));
}

/* Bullet 8: dec inside range returns -1 and is observable via get. */
static void test_saturating_counter_dec_inside_range(void) {
    saturating_counter c;
    saturating_counter_init(&c, 0, 10, 5);
    TEST_ASSERT_EQUAL_INT32(4, saturating_counter_dec(&c));
    TEST_ASSERT_EQUAL_INT32(4, saturating_counter_get(&c));
}

/* Bullet 9: dec at min saturates (no wrap) and stays at min across repeats. */
static void test_saturating_counter_dec_saturates_at_min(void) {
    saturating_counter c;
    saturating_counter_init(&c, 0, 10, 0);
    TEST_ASSERT_EQUAL_INT32(0, saturating_counter_dec(&c));
    TEST_ASSERT_EQUAL_INT32(0, saturating_counter_dec(&c));
    TEST_ASSERT_EQUAL_INT32(0, saturating_counter_get(&c));
}

/* Bullet 10: inc at INT32_MAX guards before incrementing (no signed overflow UB). */
static void test_saturating_counter_inc_no_overflow_at_int32_max(void) {
    saturating_counter c;
    saturating_counter_init(&c, 0, INT32_MAX, INT32_MAX);
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, saturating_counter_inc(&c));
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, saturating_counter_get(&c));
}

/* Bullet 11: dec at INT32_MIN guards before decrementing (no signed underflow UB). */
static void test_saturating_counter_dec_no_underflow_at_int32_min(void) {
    saturating_counter c;
    saturating_counter_init(&c, INT32_MIN, 0, INT32_MIN);
    TEST_ASSERT_EQUAL_INT32(INT32_MIN, saturating_counter_dec(&c));
    TEST_ASSERT_EQUAL_INT32(INT32_MIN, saturating_counter_get(&c));
}

/* Bullet 12: init(NULL) is a no-op and must not dereference NULL. */
static void test_saturating_counter_init_null_is_noop(void) {
    saturating_counter_init(NULL, 0, 10, 5); /* must return without crashing */
    TEST_PASS();
}

/* Bullet 13: inc/dec/get on NULL each return 0 and do not dereference NULL. */
static void test_saturating_counter_null_inc_dec_get_return_zero(void) {
    TEST_ASSERT_EQUAL_INT32(0, saturating_counter_inc(NULL));
    TEST_ASSERT_EQUAL_INT32(0, saturating_counter_dec(NULL));
    TEST_ASSERT_EQUAL_INT32(0, saturating_counter_get(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_saturating_counter_init_stores_value_and_bounds);
    RUN_TEST(test_saturating_counter_init_clamps_below_min);
    RUN_TEST(test_saturating_counter_init_clamps_above_max);
    RUN_TEST(test_saturating_counter_init_swaps_inverted_bounds);
    RUN_TEST(test_saturating_counter_degenerate_range_pins_value);
    RUN_TEST(test_saturating_counter_inc_inside_range);
    RUN_TEST(test_saturating_counter_inc_saturates_at_max);
    RUN_TEST(test_saturating_counter_dec_inside_range);
    RUN_TEST(test_saturating_counter_dec_saturates_at_min);
    RUN_TEST(test_saturating_counter_inc_no_overflow_at_int32_max);
    RUN_TEST(test_saturating_counter_dec_no_underflow_at_int32_min);
    RUN_TEST(test_saturating_counter_init_null_is_noop);
    RUN_TEST(test_saturating_counter_null_inc_dec_get_return_zero);
    return UNITY_END();
}