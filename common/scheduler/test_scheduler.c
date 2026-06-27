/* Unity host test for `scheduler` (cooperative tick-driven task scheduler).
 *
 * RED step: this test exercises the frozen API contract only. No
 * implementation (scheduler.c) exists yet, so the link step must fail with
 * undefined references to scheduler_init / scheduler_add / scheduler_remove /
 * scheduler_tick / scheduler_count. That failure is the expected RED — do NOT
 * add production code here.
 *
 * Contract under test (from the batch-3 design spec, `CLIBS_SCHEDULER_H`):
 *   typedef void (*scheduler_task_fn)(void *ctx);
 *   void   scheduler_init(scheduler *self, scheduler_task *storage, size_t cap);
 *   int    scheduler_add(scheduler *self, scheduler_task_fn fn, void *ctx,
 *                        uint32_t period);  // id >=0, or -1 on full/bad args
 *   bool   scheduler_remove(scheduler *self, int task_id);
 *   void   scheduler_tick(scheduler *self); // now++; run tasks whose due reached
 *   size_t scheduler_count(const scheduler *self);
 */
#include "unity.h"
#include "scheduler.h" /* header from the frozen API contract (impl does not exist yet) */

/* ctx counters: each task bumps its own counter so a test can assert exactly
 * how many times a callback fired. */
static int ctr_a;
static int ctr_b;

static void fnA(void *ctx) { (void)ctx; ctr_a++; }
static void fnB(void *ctx) { (void)ctx; ctr_b++; }

void setUp(void)    { ctr_a = 0; ctr_b = 0; }
void tearDown(void) {}

/* Bullet 1: scheduler_task slots[4], init(self, slots, 4) -> count 0. */
static void test_scheduler_init_sets_count_zero(void) {
    scheduler_task slots[4];
    scheduler s;
    scheduler_init(&s, slots, 4);
    TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)scheduler_count(&s));
}

/* Bullet 2: add(fnA, &ctr, 2) -> id 0, count 1 (next_due = now + period). */
static void test_scheduler_add_returns_id_zero_and_increments_count(void) {
    scheduler_task slots[4];
    scheduler s;
    scheduler_init(&s, slots, 4);

    int id = scheduler_add(&s, fnA, &ctr_a, 2);
    TEST_ASSERT_EQUAL_INT(0, id);
    TEST_ASSERT_EQUAL_UINT32(1u, (uint32_t)scheduler_count(&s));
    /* next_due = now + period (now starts at 0, period 2): the task must NOT
     * fire on tick 1, only from tick 2 onward — proven in the cadence tests. */
}

/* Bullet 3: single period-2 task: tick() x6 -> fnA invoked exactly 3 times
 * (ticks 2,4,6). Also assert it does NOT fire on the odd ticks in between. */
static void test_scheduler_single_period2_fires_on_even_ticks(void) {
    scheduler_task slots[4];
    scheduler s;
    scheduler_init(&s, slots, 4);
    scheduler_add(&s, fnA, &ctr_a, 2);

    int expected[6] = { 0, 1, 1, 2, 2, 3 }; /* cumulative count after each tick */
    for (int i = 0; i < 6; i++) {
        scheduler_tick(&s);
        TEST_ASSERT_EQUAL_INT(expected[i], ctr_a);
    }
    TEST_ASSERT_EQUAL_INT(3, ctr_a); /* fired at ticks 2,4,6 */
}

/* Bullet 4: period-2 task A + period-3 task B: tick() x6 ->
 *   A runs 3x (ticks 2,4,6), B runs 2x (ticks 3,6).
 * Each run advances next_due += period (steady cadence — verified by the
 * per-tick cumulative pattern, not just the totals). */
static void test_scheduler_two_tasks_independent_cadence(void) {
    scheduler_task slots[4];
    scheduler s;
    scheduler_init(&s, slots, 4);
    scheduler_add(&s, fnA, &ctr_a, 2); /* fires at 2,4,6 */
    scheduler_add(&s, fnB, &ctr_b, 3); /* fires at 3,6   */

    int exp_a[6] = { 0, 1, 1, 2, 2, 3 };
    int exp_b[6] = { 0, 0, 1, 1, 1, 2 };
    for (int i = 0; i < 6; i++) {
        scheduler_tick(&s);
        TEST_ASSERT_EQUAL_INT(exp_a[i], ctr_a);
        TEST_ASSERT_EQUAL_INT(exp_b[i], ctr_b);
    }
    TEST_ASSERT_EQUAL_INT(3, ctr_a);
    TEST_ASSERT_EQUAL_INT(2, ctr_b);
    TEST_ASSERT_EQUAL_UINT32(2u, (uint32_t)scheduler_count(&s));
}

/* Bullet 5: remove(id) -> task no longer runs, count decremented (slot freed). */
static void test_scheduler_remove_stops_task_and_decrements_count(void) {
    scheduler_task slots[4];
    scheduler s;
    scheduler_init(&s, slots, 4);
    int idA = scheduler_add(&s, fnA, &ctr_a, 2);
    scheduler_add(&s, fnB, &ctr_b, 2);
    TEST_ASSERT_EQUAL_UINT32(2u, (uint32_t)scheduler_count(&s));

    TEST_ASSERT_TRUE(scheduler_remove(&s, idA));
    TEST_ASSERT_EQUAL_UINT32(1u, (uint32_t)scheduler_count(&s)); /* slot freed */

    /* A is gone, B still runs: 4 ticks -> A stays 0, B fires at 2,4 (=2). */
    for (int i = 0; i < 4; i++) {
        scheduler_tick(&s);
    }
    TEST_ASSERT_EQUAL_INT(0, ctr_a); /* removed task never ran */
    TEST_ASSERT_EQUAL_INT(2, ctr_b);
}

/* Bullet 6: edge: add beyond cap -> -1; add(.,.,0) period 0 -> -1. */
static void test_scheduler_add_rejects_full_table_and_zero_period(void) {
    scheduler_task slots[4];
    scheduler s;
    scheduler_init(&s, slots, 4);

    /* Fill all 4 slots. */
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(i, scheduler_add(&s, fnA, &ctr_a, 1));
    }
    TEST_ASSERT_EQUAL_UINT32(4u, (uint32_t)scheduler_count(&s));

    /* 5th add -> table full -> -1. */
    TEST_ASSERT_EQUAL_INT(-1, scheduler_add(&s, fnA, &ctr_a, 1));

    /* period 0 is invalid even with room (fresh scheduler). */
    scheduler s2;
    scheduler_task slots2[4];
    scheduler_init(&s2, slots2, 4);
    TEST_ASSERT_EQUAL_INT(-1, scheduler_add(&s2, fnA, &ctr_a, 0));
    TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)scheduler_count(&s2)); /* not stored */
}

/* Bullet 7: NULL/edge —
 *   scheduler_add(NULL, ..)    -> -1
 *   scheduler_tick(NULL)       -> no crash (no-op)
 *   scheduler_count(NULL)      -> 0
 *   scheduler_remove(NULL, id) -> false
 */
static void test_scheduler_null_self_is_safe(void) {
    TEST_ASSERT_EQUAL_INT(-1, scheduler_add(NULL, fnA, &ctr_a, 2));
    scheduler_tick(NULL); /* must not crash */
    TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)scheduler_count(NULL));
    TEST_ASSERT_FALSE(scheduler_remove(NULL, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scheduler_init_sets_count_zero);
    RUN_TEST(test_scheduler_add_returns_id_zero_and_increments_count);
    RUN_TEST(test_scheduler_single_period2_fires_on_even_ticks);
    RUN_TEST(test_scheduler_two_tasks_independent_cadence);
    RUN_TEST(test_scheduler_remove_stops_task_and_decrements_count);
    RUN_TEST(test_scheduler_add_rejects_full_table_and_zero_period);
    RUN_TEST(test_scheduler_null_self_is_safe);
    return UNITY_END();
}
