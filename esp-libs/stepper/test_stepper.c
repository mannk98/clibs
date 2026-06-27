/* RED test for the stepper (28BYJ-48 half-step sequencer) L3 pure lib.
 * Tests ONLY the stepper.h API contract; the header/impl do not exist yet -> RED.
 * 4-wire unipolar half-step table (8): {0x01,0x03,0x02,0x06,0x04,0x0C,0x08,0x09}
 * indexed by phase; step(forward): phase = forward ? (phase+1)&7 : (phase+7)&7,
 * position += forward?1:-1, returns HALF[phase].
 * One test per host-testable acceptance bullet. No malloc; stack storage only.
 * Behavioural assertions through the public API (struct fields are private). */
#include "unity.h"
#include "stepper.h"   /* header does not exist yet -> RED */

#include <stdint.h>
#include <stdbool.h>

void setUp(void) {}
void tearDown(void) {}

/* Bullet 1: a fresh stepper after stepper_init has coil pattern 0x01
 * (HALF[0]) and position 0. */
static void test_stepper_init_coils_0x01_position_0(void) {
    stepper s;
    stepper_init(&s);

    TEST_ASSERT_EQUAL_HEX8(0x01, stepper_coils(&s));
    TEST_ASSERT_EQUAL_INT32(0, stepper_position(&s));
}

/* Bullet 2: from init, stepper_step(true) called 8 times returns the full
 * forward half-step cycle 0x03,0x02,0x06,0x04,0x0C,0x08,0x09,0x01 in order;
 * after 8 steps position == 8 and coils wraps back to 0x01 (HALF[0]). */
static void test_stepper_step_forward_8x_full_cycle(void) {
    stepper s;
    stepper_init(&s);

    TEST_ASSERT_EQUAL_HEX8(0x03, stepper_step(&s, true)); /* phase 0 -> 1 */
    TEST_ASSERT_EQUAL_HEX8(0x02, stepper_step(&s, true)); /* phase 1 -> 2 */
    TEST_ASSERT_EQUAL_HEX8(0x06, stepper_step(&s, true)); /* phase 2 -> 3 */
    TEST_ASSERT_EQUAL_HEX8(0x04, stepper_step(&s, true)); /* phase 3 -> 4 */
    TEST_ASSERT_EQUAL_HEX8(0x0C, stepper_step(&s, true)); /* phase 4 -> 5 */
    TEST_ASSERT_EQUAL_HEX8(0x08, stepper_step(&s, true)); /* phase 5 -> 6 */
    TEST_ASSERT_EQUAL_HEX8(0x09, stepper_step(&s, true)); /* phase 6 -> 7 */
    TEST_ASSERT_EQUAL_HEX8(0x01, stepper_step(&s, true)); /* phase 7 -> 0 */

    TEST_ASSERT_EQUAL_INT32(8, stepper_position(&s));
    TEST_ASSERT_EQUAL_HEX8(0x01, stepper_coils(&s));
}

/* Bullet 3: a fresh stepper stepped backward once wraps phase 0 -> 7,
 * returning 0x09 (HALF[7]) and leaving position at -1. */
static void test_stepper_step_backward_once_wraps_to_0x09(void) {
    stepper s;
    stepper_init(&s);

    TEST_ASSERT_EQUAL_HEX8(0x09, stepper_step(&s, false)); /* phase 0 -> 7 */
    TEST_ASSERT_EQUAL_INT32(-1, stepper_position(&s));
}

/* Bullet 4: NULL self -> stepper_step returns 0, stepper_coils returns 0,
 * stepper_position returns 0, and stepper_init is a no-op (no crash). */
static void test_stepper_null_self_is_safe(void) {
    stepper_init(NULL); /* no-op, must not crash */

    TEST_ASSERT_EQUAL_HEX8(0x00, stepper_step(NULL, true));
    TEST_ASSERT_EQUAL_HEX8(0x00, stepper_coils(NULL));
    TEST_ASSERT_EQUAL_INT32(0, stepper_position(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stepper_init_coils_0x01_position_0);
    RUN_TEST(test_stepper_step_forward_8x_full_cycle);
    RUN_TEST(test_stepper_step_backward_once_wraps_to_0x09);
    RUN_TEST(test_stepper_null_self_is_safe);
    return UNITY_END();
}
