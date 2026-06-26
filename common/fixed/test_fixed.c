/* Unity host test for `fixed` (Q16.16 value type) — RED step.
 *
 * Tests ONLY the frozen API contract from the batch-1 design spec; the
 * implementation (fixed.c) does not exist yet, so the link MUST fail with an
 * undefined-reference error. The header (fixed.h) is part of the frozen
 * contract and is expected to exist.
 */
#include "unity.h"
#include "fixed.h"

void setUp(void) {}
void tearDown(void) {}

/* Acceptance 1:
 *   fixed_from_int(3)==3*FIXED_ONE;
 *   fixed_to_int(fixed_from_int(3))==3;
 *   fixed_to_int(fixed_from_int(-3))==-3.
 */
static void test_fixed_from_to_int_roundtrip(void) {
    TEST_ASSERT_EQUAL_INT32(3 * FIXED_ONE, fixed_from_int(3));
    TEST_ASSERT_EQUAL_INT32(3, fixed_to_int(fixed_from_int(3)));
    TEST_ASSERT_EQUAL_INT32(-3, fixed_to_int(fixed_from_int(-3)));
}

/* Acceptance 2 — toward-zero truncation:
 *   fixed_to_int(fixed_from_float(2.9f))==2;
 *   fixed_to_int(fixed_from_float(-2.5f))==-2.
 */
static void test_fixed_to_int_truncates_toward_zero(void) {
    TEST_ASSERT_EQUAL_INT32(2, fixed_to_int(fixed_from_float(2.9f)));
    TEST_ASSERT_EQUAL_INT32(-2, fixed_to_int(fixed_from_float(-2.5f)));
}

/* Acceptance 3:
 *   fixed_add(from_int(3),from_int(4))==from_int(7);
 *   fixed_sub(from_int(3),from_int(4))==from_int(-1).
 */
static void test_fixed_add_sub(void) {
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(7),
                            fixed_add(fixed_from_int(3), fixed_from_int(4)));
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(-1),
                            fixed_sub(fixed_from_int(3), fixed_from_int(4)));
}

/* Acceptance 4:
 *   fixed_mul(from_int(3),from_int(4))==from_int(12);
 *   fixed_div(from_int(12),from_int(4))==from_int(3).
 */
static void test_fixed_mul_div(void) {
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(12),
                            fixed_mul(fixed_from_int(3), fixed_from_int(4)));
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(3),
                            fixed_div(fixed_from_int(12), fixed_from_int(4)));
}

/* Acceptance 5:
 *   fixed_div(from_int(5),0)==0 (divide-by-zero returns 0).
 */
static void test_fixed_div_by_zero_returns_zero(void) {
    TEST_ASSERT_EQUAL_INT32(0, fixed_div(fixed_from_int(5), 0));
}

/* Acceptance 6:
 *   fixed_from_float(1.5f)==98304;
 *   fixed_to_float(98304) within 0.001 of 1.5f.
 */
static void test_fixed_float_conversions(void) {
    TEST_ASSERT_EQUAL_INT32(98304, fixed_from_float(1.5f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, fixed_to_float(98304));
}

/* Acceptance 7:
 *   fixed_sqrt(from_int(4))==from_int(2);
 *   fixed_sqrt(from_int(2)) within 2 LSB of fixed_from_float(1.4142135f);
 *   fixed_sqrt(from_int(-1))==0.
 */
static void test_fixed_sqrt(void) {
    TEST_ASSERT_EQUAL_INT32(fixed_from_int(2), fixed_sqrt(fixed_from_int(4)));

    fixed_t expected = fixed_from_float(1.4142135f);
    fixed_t got = fixed_sqrt(fixed_from_int(2));
    fixed_t diff = got - expected;
    if (diff < 0) diff = -diff;
    TEST_ASSERT_TRUE(diff <= 2);                 /* within 2 LSB */

    TEST_ASSERT_EQUAL_INT32(0, fixed_sqrt(fixed_from_int(-1)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fixed_from_to_int_roundtrip);
    RUN_TEST(test_fixed_to_int_truncates_toward_zero);
    RUN_TEST(test_fixed_add_sub);
    RUN_TEST(test_fixed_mul_div);
    RUN_TEST(test_fixed_div_by_zero_returns_zero);
    RUN_TEST(test_fixed_float_conversions);
    RUN_TEST(test_fixed_sqrt);
    return UNITY_END();
}
