/* Unity host test for `softtimer` — wrap-safe poll-based countdown timer.
 *
 * RED step (TDD): this test is written against the FROZEN API contract only
 * (see docs/superpowers/specs/2026-06-27-clibs-common-oop-batch4-design.md,
 * "Module specs -> softtimer"). No implementation (softtimer.c) and no header
 * (softtimer.h) exist yet, so this MUST fail to build/link until the Dev
 * provides them — that failure is the expected RED.
 *
 * Contract under test (header does not exist yet — declared by the contract):
 *   typedef struct {                 // fields private
 *       uint32_t now;
 *       uint32_t expiry;
 *       uint32_t period;             // 0 = one-shot
 *       bool     running;
 *   } softtimer;
 *
 *   void     softtimer_init     (softtimer *self);
 *   void     softtimer_start    (softtimer *self, uint32_t duration, bool repeating);
 *   bool     softtimer_update   (softtimer *self, uint32_t elapsed);   // now += elapsed; true if fired
 *   bool     softtimer_running  (const softtimer *self);
 *   void     softtimer_stop     (softtimer *self);
 *   uint32_t softtimer_remaining(const softtimer *self);  // 0 if not running / already due
 *
 * Semantics (spec): start -> expiry = now + duration; period = repeating?duration:0;
 * running = true. update -> now += elapsed; if running && (int32_t)(now - expiry) >= 0
 * it fired; if period != 0 expiry += period (auto-reload, one fire per update) else
 * running = false (one-shot). The signed-difference compare is wrap-safe across a
 * uint32_t rollover. No callback (poll the return value).
 */
#include "softtimer.h"
#include "unity.h"

#include <stdint.h>
#include <stdbool.h>

void setUp(void)    {}
void tearDown(void) {}

/* Acceptance #1:
 * init -> running false, remaining 0. */
void test_softtimer_init_not_running_remaining_zero(void)
{
    softtimer t;
    softtimer_init(&t);

    TEST_ASSERT_FALSE(softtimer_running(&t));
    TEST_ASSERT_EQUAL_UINT32(0u, softtimer_remaining(&t));
}

/* Acceptance #2:
 * start(100, false) -> running true, remaining 100. */
void test_softtimer_start_sets_running_and_remaining(void)
{
    softtimer t;
    softtimer_init(&t);

    softtimer_start(&t, 100u, false);

    TEST_ASSERT_TRUE(softtimer_running(&t));
    TEST_ASSERT_EQUAL_UINT32(100u, softtimer_remaining(&t));
}

/* Acceptance #3:
 * one-shot(100): update(50) -> false, remaining 50; update(50) -> true (fires),
 * then running false. */
void test_softtimer_one_shot_fires_once_then_stops(void)
{
    softtimer t;
    softtimer_init(&t);
    softtimer_start(&t, 100u, false);

    TEST_ASSERT_FALSE(softtimer_update(&t, 50u));
    TEST_ASSERT_EQUAL_UINT32(50u, softtimer_remaining(&t));

    TEST_ASSERT_TRUE(softtimer_update(&t, 50u));   /* now == expiry -> fires */
    TEST_ASSERT_FALSE(softtimer_running(&t));       /* one-shot is done */
}

/* Acceptance #4:
 * after stop/one-shot done: update(10) -> false (no further firing). */
void test_softtimer_no_firing_after_one_shot_done(void)
{
    softtimer t;
    softtimer_init(&t);
    softtimer_start(&t, 100u, false);

    (void)softtimer_update(&t, 100u);               /* fires, one-shot done */
    TEST_ASSERT_FALSE(softtimer_running(&t));

    TEST_ASSERT_FALSE(softtimer_update(&t, 10u));   /* stopped -> never fires again */
    TEST_ASSERT_FALSE(softtimer_running(&t));
    TEST_ASSERT_EQUAL_UINT32(0u, softtimer_remaining(&t));
}

/* Acceptance #5:
 * repeating: start(100, true); update(100) -> true, running stays true, remaining 100;
 * update(100) -> true again (auto-reload, one fire per update). */
void test_softtimer_repeating_auto_reloads(void)
{
    softtimer t;
    softtimer_init(&t);
    softtimer_start(&t, 100u, true);

    TEST_ASSERT_TRUE(softtimer_update(&t, 100u));   /* fires */
    TEST_ASSERT_TRUE(softtimer_running(&t));         /* still armed (repeating) */
    TEST_ASSERT_EQUAL_UINT32(100u, softtimer_remaining(&t)); /* auto-reloaded full period */

    TEST_ASSERT_TRUE(softtimer_update(&t, 100u));   /* fires again next period */
    TEST_ASSERT_TRUE(softtimer_running(&t));
}

/* Acceptance #6:
 * stop(): running false, remaining 0. */
void test_softtimer_stop_clears_running_and_remaining(void)
{
    softtimer t;
    softtimer_init(&t);
    softtimer_start(&t, 100u, true);
    TEST_ASSERT_TRUE(softtimer_running(&t));

    softtimer_stop(&t);

    TEST_ASSERT_FALSE(softtimer_running(&t));
    TEST_ASSERT_EQUAL_UINT32(0u, softtimer_remaining(&t));
}

/* Acceptance #7:
 * wrap-safe: update(0xFFFFFF00) to advance now near max, start(0x200, false)
 * (expiry wraps past 0), update(0x100) -> false, update(0x100) -> true (fires
 * across the uint32 wrap via signed-diff compare). */
void test_softtimer_wrap_safe_fires_across_uint32_rollover(void)
{
    softtimer t;
    softtimer_init(&t);

    /* Advance internal `now` to near the top of the uint32 range. The timer is
     * not running yet, so this update neither fires nor matters semantically —
     * it just positions `now` so the next expiry crosses the wrap boundary. */
    TEST_ASSERT_FALSE(softtimer_update(&t, 0xFFFFFF00u));

    /* now ~= 0xFFFFFF00; expiry = now + 0x200 wraps past 0 to ~0x100. */
    softtimer_start(&t, 0x200u, false);
    TEST_ASSERT_TRUE(softtimer_running(&t));

    /* now -> ~0x00000000 (wrapped): still before expiry by signed-diff. */
    TEST_ASSERT_FALSE(softtimer_update(&t, 0x100u));
    TEST_ASSERT_TRUE(softtimer_running(&t));

    /* now -> ~0x00000100 == expiry: fires correctly despite the wrap. */
    TEST_ASSERT_TRUE(softtimer_update(&t, 0x100u));
    TEST_ASSERT_FALSE(softtimer_running(&t));   /* one-shot done */
}

/* Acceptance #8:
 * NULL self: softtimer_update(NULL,..) false; softtimer_running(NULL) false;
 * softtimer_remaining(NULL) 0; softtimer_start(NULL,..)/init(NULL)/stop(NULL)
 * no crash. */
void test_softtimer_null_self_is_safe(void)
{
    /* Pointer-returning / bool-returning queries have defined NULL behaviour. */
    TEST_ASSERT_FALSE(softtimer_update(NULL, 10u));
    TEST_ASSERT_FALSE(softtimer_running(NULL));
    TEST_ASSERT_EQUAL_UINT32(0u, softtimer_remaining(NULL));

    /* void functions must be no-ops on NULL self (must not crash). */
    softtimer_init(NULL);
    softtimer_start(NULL, 100u, true);
    softtimer_stop(NULL);

    TEST_PASS();   /* reaching here without a crash satisfies the no-crash clause */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_softtimer_init_not_running_remaining_zero);
    RUN_TEST(test_softtimer_start_sets_running_and_remaining);
    RUN_TEST(test_softtimer_one_shot_fires_once_then_stops);
    RUN_TEST(test_softtimer_no_firing_after_one_shot_done);
    RUN_TEST(test_softtimer_repeating_auto_reloads);
    RUN_TEST(test_softtimer_stop_clears_running_and_remaining);
    RUN_TEST(test_softtimer_wrap_safe_fires_across_uint32_rollover);
    RUN_TEST(test_softtimer_null_self_is_safe);
    return UNITY_END();
}
