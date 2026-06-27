/* Unity host test for `event` (small publish/subscribe bus).
 *
 * RED step: this test exercises the frozen API contract only. No
 * implementation (event.c) — and not even the header (event.h) — exists
 * yet, so the build must fail: either a missing-header compile error
 * ("event.h: No such file or directory") or, once the header lands, a
 * link error with undefined references to event_init / event_subscribe /
 * event_unsubscribe / event_publish / event_count. That failure is the
 * expected RED — do NOT add production code (event.c / event.h) here.
 *
 * Contract under test (from the batch-3 design spec, `CLIBS_EVENT_H`):
 *   typedef void (*event_handler_fn)(int event_id, void *data, void *ctx);
 *   typedef struct { int event_id; event_handler_fn fn; void *ctx;
 *                    bool active; } event_sub;
 *   typedef struct { event_sub *subs; size_t cap; size_t count; } event_bus;
 *   void   event_init(event_bus *self, event_sub *storage, size_t cap);
 *   int    event_subscribe(event_bus *self, int event_id,
 *                          event_handler_fn fn, void *ctx); // id>=0, -1 bad
 *   bool   event_unsubscribe(event_bus *self, int sub_id);
 *   size_t event_publish(event_bus *self, int event_id, void *data); // # invoked
 *   size_t event_count(const event_bus *self);
 */
#include "unity.h"
#include "event.h" /* header from the frozen API contract (impl does not exist yet) */

/* Shared invocation recorder. Every handler bumps the matching counter and
 * captures the (event_id, data, ctx) it was called with, so tests can assert
 * not only "how many ran" but "the right handlers ran with the right args". */
typedef struct {
    int   calls_a, calls_b, calls_c;
    int   last_event_id_a;       /* event_id seen by hA on its last call */
    void *last_data_a;           /* data ptr seen by hA on its last call */
    void *last_ctx_a;            /* ctx   seen by hA on its last call */
} rec;

static void hA(int event_id, void *data, void *ctx) {
    rec *r = (rec *)ctx;
    r->calls_a++;
    r->last_event_id_a = event_id;
    r->last_data_a = data;
    r->last_ctx_a = ctx;
}
static void hB(int event_id, void *data, void *ctx) {
    (void)event_id; (void)data;
    ((rec *)ctx)->calls_b++;
}
static void hC(int event_id, void *data, void *ctx) {
    (void)event_id; (void)data;
    ((rec *)ctx)->calls_c++;
}

void setUp(void)    {}
void tearDown(void) {}

/* Bullet 1: event_sub slots[4], init(self,slots,4) -> count 0. */
static void test_event_init_zero_count(void) {
    event_sub slots[4];
    event_bus bus;
    event_init(&bus, slots, 4);
    TEST_ASSERT_EQUAL_size_t(0, event_count(&bus));
}

/* Bullet 2: subscribe(1,hA,&ctr)->id0; subscribe(1,hB,&ctr)->id1;
 *           subscribe(2,hC,&ctr)->id2; count 3. */
static void test_event_subscribe_returns_sequential_ids_and_counts(void) {
    rec ctr = {0};
    event_sub slots[4];
    event_bus bus;
    event_init(&bus, slots, 4);

    TEST_ASSERT_EQUAL_INT(0, event_subscribe(&bus, 1, hA, &ctr));
    TEST_ASSERT_EQUAL_INT(1, event_subscribe(&bus, 1, hB, &ctr));
    TEST_ASSERT_EQUAL_INT(2, event_subscribe(&bus, 2, hC, &ctr));
    TEST_ASSERT_EQUAL_size_t(3, event_count(&bus));
}

/* Bullet 3: publish(self,1,data) -> returns 2 (hA,hB each invoked with
 *           event_id 1, the data ptr, and their ctx). */
static void test_event_publish_invokes_all_matching_subs(void) {
    rec ctr = {0};
    int payload = 42;
    void *data = &payload;
    event_sub slots[4];
    event_bus bus;
    event_init(&bus, slots, 4);
    event_subscribe(&bus, 1, hA, &ctr);
    event_subscribe(&bus, 1, hB, &ctr);
    event_subscribe(&bus, 2, hC, &ctr);

    TEST_ASSERT_EQUAL_size_t(2, event_publish(&bus, 1, data));
    /* both event-1 handlers ran exactly once; the event-2 handler did not */
    TEST_ASSERT_EQUAL_INT(1, ctr.calls_a);
    TEST_ASSERT_EQUAL_INT(1, ctr.calls_b);
    TEST_ASSERT_EQUAL_INT(0, ctr.calls_c);
    /* hA received the right (event_id, data, ctx) triple */
    TEST_ASSERT_EQUAL_INT(1, ctr.last_event_id_a);
    TEST_ASSERT_EQUAL_PTR(data, ctr.last_data_a);
    TEST_ASSERT_EQUAL_PTR(&ctr, ctr.last_ctx_a);
}

/* Bullet 4: publish(self,2,data) -> 1; publish(self,3,data) -> 0 (no subs). */
static void test_event_publish_counts_by_event_id(void) {
    rec ctr = {0};
    int payload = 7;
    event_sub slots[4];
    event_bus bus;
    event_init(&bus, slots, 4);
    event_subscribe(&bus, 1, hA, &ctr);
    event_subscribe(&bus, 1, hB, &ctr);
    event_subscribe(&bus, 2, hC, &ctr);

    TEST_ASSERT_EQUAL_size_t(1, event_publish(&bus, 2, &payload));
    TEST_ASSERT_EQUAL_INT(1, ctr.calls_c);          /* only hC ran */
    TEST_ASSERT_EQUAL_INT(0, ctr.calls_a);
    TEST_ASSERT_EQUAL_INT(0, ctr.calls_b);

    /* no subscriber for event 3 -> zero invoked, nothing fires */
    TEST_ASSERT_EQUAL_size_t(0, event_publish(&bus, 3, &payload));
    TEST_ASSERT_EQUAL_INT(1, ctr.calls_c);          /* unchanged */
}

/* Bullet 5: unsubscribe(self,0) then publish(self,1,data) -> 1 (only hB),
 *           count 2. */
static void test_event_unsubscribe_removes_one_sub(void) {
    rec ctr = {0};
    int payload = 9;
    event_sub slots[4];
    event_bus bus;
    event_init(&bus, slots, 4);
    event_subscribe(&bus, 1, hA, &ctr);   /* id 0 */
    event_subscribe(&bus, 1, hB, &ctr);   /* id 1 */
    event_subscribe(&bus, 2, hC, &ctr);   /* id 2 */

    TEST_ASSERT_TRUE(event_unsubscribe(&bus, 0));   /* drop hA */
    TEST_ASSERT_EQUAL_size_t(2, event_count(&bus));

    /* now only hB matches event 1 */
    TEST_ASSERT_EQUAL_size_t(1, event_publish(&bus, 1, &payload));
    TEST_ASSERT_EQUAL_INT(0, ctr.calls_a);          /* hA gone */
    TEST_ASSERT_EQUAL_INT(1, ctr.calls_b);          /* hB still fires */
}

/* Bullet 6 (edge): subscribe beyond cap -> -1; subscribe(.,.,NULL,.) NULL fn -> -1. */
static void test_event_subscribe_rejects_full_and_null_fn(void) {
    rec ctr = {0};
    event_sub slots[4];
    event_bus bus;
    event_init(&bus, slots, 4);

    /* fill all 4 slots */
    TEST_ASSERT_EQUAL_INT(0, event_subscribe(&bus, 1, hA, &ctr));
    TEST_ASSERT_EQUAL_INT(1, event_subscribe(&bus, 1, hB, &ctr));
    TEST_ASSERT_EQUAL_INT(2, event_subscribe(&bus, 2, hC, &ctr));
    TEST_ASSERT_EQUAL_INT(3, event_subscribe(&bus, 3, hA, &ctr));
    TEST_ASSERT_EQUAL_size_t(4, event_count(&bus));

    /* table full -> -1, count unchanged */
    TEST_ASSERT_EQUAL_INT(-1, event_subscribe(&bus, 4, hB, &ctr));
    TEST_ASSERT_EQUAL_size_t(4, event_count(&bus));

    /* NULL handler fn rejected on a fresh (non-full) bus -> -1 */
    event_sub slots2[4];
    event_bus bus2;
    event_init(&bus2, slots2, 4);
    TEST_ASSERT_EQUAL_INT(-1, event_subscribe(&bus2, 1, NULL, &ctr));
    TEST_ASSERT_EQUAL_size_t(0, event_count(&bus2));
}

/* Bullet 7 (NULL/edge): event_subscribe(NULL,..) -1; event_publish(NULL,..) 0;
 *           event_count(NULL) 0; event_unsubscribe(NULL,id) false. */
static void test_event_null_self_is_safe(void) {
    rec ctr = {0};
    int payload = 1;
    TEST_ASSERT_EQUAL_INT(-1, event_subscribe(NULL, 1, hA, &ctr));
    TEST_ASSERT_EQUAL_size_t(0, event_publish(NULL, 1, &payload));
    TEST_ASSERT_EQUAL_size_t(0, event_count(NULL));
    TEST_ASSERT_FALSE(event_unsubscribe(NULL, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_event_init_zero_count);
    RUN_TEST(test_event_subscribe_returns_sequential_ids_and_counts);
    RUN_TEST(test_event_publish_invokes_all_matching_subs);
    RUN_TEST(test_event_publish_counts_by_event_id);
    RUN_TEST(test_event_unsubscribe_removes_one_sub);
    RUN_TEST(test_event_subscribe_rejects_full_and_null_fn);
    RUN_TEST(test_event_null_self_is_safe);
    return UNITY_END();
}
