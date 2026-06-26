/* Unity host test for `fsm` (table-driven finite state machine).
 *
 * RED step: this test exercises the frozen API contract only. No
 * implementation (fsm.c) exists yet, so the link step must fail with
 * undefined references to fsm_init / fsm_dispatch / fsm_state (or, if
 * the header is also absent yet, a missing-header compile error). That
 * failure is the expected RED — do NOT add production code here.
 *
 * Contract under test (from the batch-1 design spec, `CLIBS_FSM_H`):
 *   typedef struct { int from; int event; int to;
 *                    void (*action)(void *ctx); } fsm_transition;
 *   typedef struct { const fsm_transition *table; size_t count;
 *                    int state; void *ctx; } fsm;
 *   void fsm_init(fsm *self, const fsm_transition *table,
 *                 size_t count, int initial, void *ctx);
 *   int  fsm_state(const fsm *self);          // -1 if NULL self
 *   bool fsm_dispatch(fsm *self, int event);  // first match wins:
 *                                             // action(ctx) THEN state=to
 */
#include "unity.h"
#include "fsm.h" /* header from the frozen API contract (impl does not exist yet) */

/* Turnstile model from the acceptance criteria. */
enum { LOCKED = 0, UNLOCKED = 1 };  /* state ids */
enum { COIN = 0, PUSH = 1 };        /* event ids */

/* ctx accumulates the side effects so tests can assert "the action ran". */
typedef struct {
    int unlocks;
    int locks;
} turnstile_ctx;

static void on_unlock(void *ctx) { ((turnstile_ctx *)ctx)->unlocks++; }
static void on_lock(void *ctx)   { ((turnstile_ctx *)ctx)->locks++; }

/* Turnstile transition table, exactly as specified:
 *   {L,COIN,U,on_unlock} {U,PUSH,L,on_lock} {L,PUSH,L,NULL} {U,COIN,U,NULL}
 * `const` so it can live in flash on an MCU. */
static const fsm_transition turnstile_table[] = {
    { LOCKED,   COIN, UNLOCKED, on_unlock },
    { UNLOCKED, PUSH, LOCKED,   on_lock   },
    { LOCKED,   PUSH, LOCKED,   NULL      }, /* self-loop, no action */
    { UNLOCKED, COIN, UNLOCKED, NULL      }, /* self-loop, no action */
};

void setUp(void)    {}
void tearDown(void) {}

/* Bullet 1: init(.,table,4,LOCKED,&ctx) leaves fsm_state == LOCKED. */
static void test_fsm_init_sets_initial_state_locked(void) {
    turnstile_ctx ctx = { 0, 0 };
    fsm m;
    fsm_init(&m, turnstile_table, 4, LOCKED, &ctx);
    TEST_ASSERT_EQUAL_INT(LOCKED, fsm_state(&m));
}

/* Bullet 2: dispatch(COIN) -> true, state UNLOCKED, ctx.unlocks == 1. */
static void test_fsm_dispatch_coin_unlocks_and_runs_action(void) {
    turnstile_ctx ctx = { 0, 0 };
    fsm m;
    fsm_init(&m, turnstile_table, 4, LOCKED, &ctx);

    TEST_ASSERT_TRUE(fsm_dispatch(&m, COIN));
    TEST_ASSERT_EQUAL_INT(UNLOCKED, fsm_state(&m));
    TEST_ASSERT_EQUAL_INT(1, ctx.unlocks); /* on_unlock ran */
    TEST_ASSERT_EQUAL_INT(0, ctx.locks);
}

/* Bullet 3: from UNLOCKED, dispatch(PUSH) -> true, state LOCKED, ctx.locks == 1. */
static void test_fsm_dispatch_push_from_unlocked_locks_and_runs_action(void) {
    turnstile_ctx ctx = { 0, 0 };
    fsm m;
    fsm_init(&m, turnstile_table, 4, UNLOCKED, &ctx);

    TEST_ASSERT_TRUE(fsm_dispatch(&m, PUSH));
    TEST_ASSERT_EQUAL_INT(LOCKED, fsm_state(&m));
    TEST_ASSERT_EQUAL_INT(1, ctx.locks); /* on_lock ran */
    TEST_ASSERT_EQUAL_INT(0, ctx.unlocks);
}

/* Bullet 4: from LOCKED, dispatch(PUSH) is a self-loop with NULL action ->
 * returns true, state stays LOCKED, neither counter changes. */
static void test_fsm_dispatch_push_from_locked_self_loop_no_action(void) {
    turnstile_ctx ctx = { 0, 0 };
    fsm m;
    fsm_init(&m, turnstile_table, 4, LOCKED, &ctx);

    TEST_ASSERT_TRUE(fsm_dispatch(&m, PUSH));
    TEST_ASSERT_EQUAL_INT(LOCKED, fsm_state(&m)); /* unchanged */
    TEST_ASSERT_EQUAL_INT(0, ctx.unlocks);        /* NULL action: nothing ran */
    TEST_ASSERT_EQUAL_INT(0, ctx.locks);
}

/* Bullet 5: from LOCKED, dispatch(99) has no matching row -> returns false,
 * state unchanged. */
static void test_fsm_dispatch_unknown_event_returns_false_no_change(void) {
    turnstile_ctx ctx = { 0, 0 };
    fsm m;
    fsm_init(&m, turnstile_table, 4, LOCKED, &ctx);

    TEST_ASSERT_FALSE(fsm_dispatch(&m, 99));
    TEST_ASSERT_EQUAL_INT(LOCKED, fsm_state(&m)); /* unchanged */
    TEST_ASSERT_EQUAL_INT(0, ctx.unlocks);
    TEST_ASSERT_EQUAL_INT(0, ctx.locks);
}

/* Bullet 6: action runs BEFORE the state update on a match, and the FIRST
 * matching row wins (linear scan).
 *
 * "Action before state update": an action that inspects fsm_state(self) must
 * see the OLD state (LOCKED), proving the action fired before state = to.
 * "First match wins": a table with two rows matching (from==FROM_S, event==EV)
 * must run only the FIRST row's action and land on the FIRST row's `to`. */
static int observed_state_in_action = -999;

static void capture_state(void *ctx) {
    fsm *m = (fsm *)ctx;
    observed_state_in_action = fsm_state(m); /* must still be the OLD state */
}

static void test_fsm_action_runs_before_state_update_and_first_row_wins(void) {
    enum { FROM_S = 5, EV = 7, FIRST_TO = 8, SECOND_TO = 9 };

    /* Two rows match (FROM_S, EV); only the first must take effect. */
    static int second_action_ran;        /* must remain 0 */
    second_action_ran = 0;
    observed_state_in_action = -999;

    /* second_marker bumps a flag if it ever runs (it must not). */
    static const fsm_transition rows[] = {
        { FROM_S, EV, FIRST_TO,  capture_state }, /* first match: wins */
        { FROM_S, EV, SECOND_TO, NULL          }, /* shadowed by the first */
    };

    fsm m;
    /* ctx is the machine itself so capture_state can read fsm_state mid-dispatch. */
    fsm_init(&m, rows, 2, FROM_S, &m);

    TEST_ASSERT_TRUE(fsm_dispatch(&m, EV));
    /* action observed the OLD state -> action ran before state was updated */
    TEST_ASSERT_EQUAL_INT(FROM_S, observed_state_in_action);
    /* first matching row won -> landed on FIRST_TO, not SECOND_TO */
    TEST_ASSERT_EQUAL_INT(FIRST_TO, fsm_state(&m));
    (void)second_action_ran;
}

/* Bullet 7: NULL self -> fsm_dispatch(NULL, COIN) returns false;
 * fsm_state(NULL) returns -1. */
static void test_fsm_null_self_is_safe(void) {
    TEST_ASSERT_FALSE(fsm_dispatch(NULL, COIN));
    TEST_ASSERT_EQUAL_INT(-1, fsm_state(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fsm_init_sets_initial_state_locked);
    RUN_TEST(test_fsm_dispatch_coin_unlocks_and_runs_action);
    RUN_TEST(test_fsm_dispatch_push_from_unlocked_locks_and_runs_action);
    RUN_TEST(test_fsm_dispatch_push_from_locked_self_loop_no_action);
    RUN_TEST(test_fsm_dispatch_unknown_event_returns_false_no_change);
    RUN_TEST(test_fsm_action_runs_before_state_update_and_first_row_wins);
    RUN_TEST(test_fsm_null_self_is_safe);
    return UNITY_END();
}
