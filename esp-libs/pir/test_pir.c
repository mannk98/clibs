/* RED test for the pir L3 pure FSM (retrigger/hold-time motion detector).
 * Tests ONLY the pir.h API contract; the header/impl do not exist yet -> RED.
 * One test per host-testable acceptance bullet. No malloc; stack storage only.
 * The SDK glue (pir_gpio.{h,c}) is NOT tested here (needs driver/gpio.h; deferred).
 */
#include "unity.h"
#include "pir.h"   /* header does not exist yet -> RED (undefined reference at link) */

#include <stdint.h>
#include <stdbool.h>

void setUp(void)    {}
void tearDown(void) {}

/* Bullet 1: pir_init(&p, 1000) leaves pir_active false (idle at start). */
static void test_pir_init_starts_inactive(void) {
    pir p;
    pir_init(&p, 1000);
    TEST_ASSERT_FALSE(pir_active(&p));
}

/* Bullet 2: first motion_raw true from idle -> PIR_MOTION_START, active true. */
static void test_pir_update_motion_start_from_idle(void) {
    pir p;
    pir_init(&p, 1000);
    TEST_ASSERT_EQUAL_INT(PIR_MOTION_START, pir_update(&p, true, 0));
    TEST_ASSERT_TRUE(pir_active(&p));
}

/* Bullet 3: motion_raw false while active and within hold -> PIR_NONE, still active.
 *   after start@0 (last=0), update(false,500): 500-0=500 < 1000 -> stay active. */
static void test_pir_update_none_within_hold(void) {
    pir p;
    pir_init(&p, 1000);
    pir_update(&p, true, 0);
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(&p, false, 500));
    TEST_ASSERT_TRUE(pir_active(&p));
}

/* Bullet 4: motion_raw true while already active -> PIR_NONE, refreshes last_ms.
 *   update(true,800): already active -> PIR_NONE, last_ms becomes 800. */
static void test_pir_update_retrigger_refreshes_last(void) {
    pir p;
    pir_init(&p, 1000);
    pir_update(&p, true, 0);     /* START, last=0   */
    pir_update(&p, false, 500);  /* NONE,  within hold */
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(&p, true, 800)); /* re-trigger, last=800 */
    TEST_ASSERT_TRUE(pir_active(&p));
    /* Bullet 5: still within hold relative to the refreshed last (1700-800=900<1000). */
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(&p, false, 1700));
    TEST_ASSERT_TRUE(pir_active(&p));
}

/* Bullet 6: motion_raw false, hold elapsed since last refresh -> PIR_MOTION_END, idle.
 *   last refreshed to 800 at re-trigger; update(false,1900): 1900-800=1100 >= 1000. */
static void test_pir_update_motion_end_after_hold(void) {
    pir p;
    pir_init(&p, 1000);
    pir_update(&p, true, 0);     /* START, last=0      */
    pir_update(&p, false, 500);  /* NONE,  within hold */
    pir_update(&p, true, 800);   /* re-trigger, last=800 */
    pir_update(&p, false, 1700); /* NONE,  900<1000     */
    TEST_ASSERT_EQUAL_INT(PIR_MOTION_END, pir_update(&p, false, 1900)); /* 1100>=1000 */
    TEST_ASSERT_FALSE(pir_active(&p));
}

/* Bullet 7: after END, idle false stays PIR_NONE; a new true -> PIR_MOTION_START again. */
static void test_pir_update_restart_after_end(void) {
    pir p;
    pir_init(&p, 1000);
    pir_update(&p, true, 0);
    pir_update(&p, false, 500);
    pir_update(&p, true, 800);
    pir_update(&p, false, 1700);
    pir_update(&p, false, 1900); /* MOTION_END -> idle */
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(&p, false, 2000)); /* idle, no motion */
    TEST_ASSERT_FALSE(pir_active(&p));
    TEST_ASSERT_EQUAL_INT(PIR_MOTION_START, pir_update(&p, true, 2100));
    TEST_ASSERT_TRUE(pir_active(&p));
}

/* Bullet 8: NULL self is a no-op: pir_update(NULL,..) -> PIR_NONE; pir_active(NULL) -> false. */
static void test_pir_null_self_is_safe(void) {
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(NULL, true, 0));
    TEST_ASSERT_EQUAL_INT(PIR_NONE, pir_update(NULL, false, 0));
    TEST_ASSERT_FALSE(pir_active(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pir_init_starts_inactive);
    RUN_TEST(test_pir_update_motion_start_from_idle);
    RUN_TEST(test_pir_update_none_within_hold);
    RUN_TEST(test_pir_update_retrigger_refreshes_last);
    RUN_TEST(test_pir_update_motion_end_after_hold);
    RUN_TEST(test_pir_update_restart_after_end);
    RUN_TEST(test_pir_null_self_is_safe);
    return UNITY_END();
}
