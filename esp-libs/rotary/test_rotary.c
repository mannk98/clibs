/* RED test for the rotary (quadrature encoder) L3 pure lib.
 * Tests ONLY the rotary.h API contract; the header/impl do not exist yet -> RED.
 * Pure quarter-step Gray decode: position counts quarter-steps, ~4 per detent.
 * One test per host-testable acceptance bullet. No malloc; stack storage only.
 * Behavioural assertions through the public API (struct fields are private). */
#include "unity.h"
#include "rotary.h"   /* header does not exist yet -> RED */

#include <stdint.h>

void setUp(void) {}
void tearDown(void) {}

/* Bullet 1: init(0); a full CW detent 00->01->11->10->00 feeds
 * update(1),update(3),update(2),update(0); each valid Gray step returns
 * ROTARY_CW and adds +1, so position ends at 4. */
static void test_rotary_cw_full_detent_returns_cw_and_position_4(void) {
    rotary enc;
    rotary_init(&enc, 0);

    TEST_ASSERT_EQUAL_INT(ROTARY_CW, rotary_update(&enc, 1));  /* 00 -> 01 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CW, rotary_update(&enc, 3));  /* 01 -> 11 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CW, rotary_update(&enc, 2));  /* 11 -> 10 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CW, rotary_update(&enc, 0));  /* 10 -> 00 */

    TEST_ASSERT_EQUAL_INT32(4, rotary_position(&enc));
}

/* Bullet 2: fresh init(0); a full CCW detent 00->10->11->01->00 feeds
 * update(2),update(3),update(1),update(0); each valid Gray step returns
 * ROTARY_CCW and adds -1, so position ends at -4. */
static void test_rotary_ccw_full_detent_returns_ccw_and_position_minus_4(void) {
    rotary enc;
    rotary_init(&enc, 0);

    TEST_ASSERT_EQUAL_INT(ROTARY_CCW, rotary_update(&enc, 2));  /* 00 -> 10 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CCW, rotary_update(&enc, 3));  /* 10 -> 11 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CCW, rotary_update(&enc, 1));  /* 11 -> 01 */
    TEST_ASSERT_EQUAL_INT(ROTARY_CCW, rotary_update(&enc, 0));  /* 01 -> 00 */

    TEST_ASSERT_EQUAL_INT32(-4, rotary_position(&enc));
}

/* Bullet 3: init(0); a 2-step jump 00->11 (update(3)) is invalid -> ROTARY_NONE,
 * position stays 0; a same-state update 11->11 (update(3)) is also ROTARY_NONE. */
static void test_rotary_jump_and_same_state_are_none_no_position_change(void) {
    rotary enc;
    rotary_init(&enc, 0);

    TEST_ASSERT_EQUAL_INT(ROTARY_NONE, rotary_update(&enc, 3)); /* 00 -> 11 jump */
    TEST_ASSERT_EQUAL_INT32(0, rotary_position(&enc));

    TEST_ASSERT_EQUAL_INT(ROTARY_NONE, rotary_update(&enc, 3)); /* 11 -> 11 same */
    TEST_ASSERT_EQUAL_INT32(0, rotary_position(&enc));
}

/* Bullet 4: NULL self -> rotary_update(NULL,1) returns ROTARY_NONE and
 * rotary_position(NULL) returns 0; neither dereferences NULL. */
static void test_rotary_null_self_is_safe(void) {
    TEST_ASSERT_EQUAL_INT(ROTARY_NONE, rotary_update(NULL, 1));
    TEST_ASSERT_EQUAL_INT32(0, rotary_position(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rotary_cw_full_detent_returns_cw_and_position_4);
    RUN_TEST(test_rotary_ccw_full_detent_returns_ccw_and_position_minus_4);
    RUN_TEST(test_rotary_jump_and_same_state_are_none_no_position_change);
    RUN_TEST(test_rotary_null_self_is_safe);
    return UNITY_END();
}
