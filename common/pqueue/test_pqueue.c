/* Unity host test for pqueue (array-backed binary min-heap, story CB2-2).
 *
 * RED step: tests the frozen API contract only. The implementation
 * (pqueue.c) does not exist yet, so this MUST fail (undefined references
 * to pqueue_* symbols, or a missing pqueue.h until the Dev writes it).
 * The header pqueue.h carries the contract under test.
 *
 * Contract (from the batch-2 design spec):
 *   typedef struct { int32_t key; void *value; } pqueue_node;
 *   typedef struct { pqueue_node *nodes; size_t cap; size_t len; } pqueue;
 *   void   pqueue_init(pqueue *self, pqueue_node *storage, size_t cap);
 *   bool   pqueue_push(pqueue *self, int32_t key, void *value);
 *   bool   pqueue_pop_min(pqueue *self, int32_t *key_out, void **val_out);
 *   bool   pqueue_peek_min(const pqueue *self, int32_t *key_out, void **val_out);
 *   size_t pqueue_len(const pqueue *self);
 *   bool   pqueue_is_empty(const pqueue *self);
 *
 * No malloc: storage is a caller-provided pqueue_node array on the stack.
 */
#include "unity.h"
#include "pqueue.h"

void setUp(void)    {}
void tearDown(void) {}

/* Distinct payload objects so we can assert which value comes out by
 * identity (pointer equality), independent of key. */
static int a, b, c;

/* Bullet 1: pqueue_node storage[8]: init -> len 0, is_empty true. */
static void test_pqueue_init_empty(void)
{
    pqueue_node storage[8];
    pqueue self;

    pqueue_init(&self, storage, 8);

    TEST_ASSERT_EQUAL_size_t(0, pqueue_len(&self));
    TEST_ASSERT_TRUE(pqueue_is_empty(&self));
}

/* Bullet 2: push (5,a),(1,b),(3,c) -> len 3; peek_min -> key 1, val b,
 * no removal (len stays 3). */
static void test_pqueue_peek_min_returns_min_without_removal(void)
{
    pqueue_node storage[8];
    pqueue self;
    int32_t key_out = -999;
    void   *val_out = NULL;

    pqueue_init(&self, storage, 8);

    TEST_ASSERT_TRUE(pqueue_push(&self, 5, &a));
    TEST_ASSERT_TRUE(pqueue_push(&self, 1, &b));
    TEST_ASSERT_TRUE(pqueue_push(&self, 3, &c));
    TEST_ASSERT_EQUAL_size_t(3, pqueue_len(&self));

    TEST_ASSERT_TRUE(pqueue_peek_min(&self, &key_out, &val_out));
    TEST_ASSERT_EQUAL_INT32(1, key_out);
    TEST_ASSERT_EQUAL_PTR(&b, val_out);

    /* peek does not remove */
    TEST_ASSERT_EQUAL_size_t(3, pqueue_len(&self));
    TEST_ASSERT_FALSE(pqueue_is_empty(&self));
}

/* Bullet 3: pop_min repeatedly -> keys 1,3,5 ascending with values b,c,a;
 * after 3 pops is_empty true. */
static void test_pqueue_pop_min_yields_ascending_keys(void)
{
    pqueue_node storage[8];
    pqueue self;
    int32_t key_out;
    void   *val_out;

    pqueue_init(&self, storage, 8);
    TEST_ASSERT_TRUE(pqueue_push(&self, 5, &a));
    TEST_ASSERT_TRUE(pqueue_push(&self, 1, &b));
    TEST_ASSERT_TRUE(pqueue_push(&self, 3, &c));

    key_out = 0; val_out = NULL;
    TEST_ASSERT_TRUE(pqueue_pop_min(&self, &key_out, &val_out));
    TEST_ASSERT_EQUAL_INT32(1, key_out);
    TEST_ASSERT_EQUAL_PTR(&b, val_out);

    key_out = 0; val_out = NULL;
    TEST_ASSERT_TRUE(pqueue_pop_min(&self, &key_out, &val_out));
    TEST_ASSERT_EQUAL_INT32(3, key_out);
    TEST_ASSERT_EQUAL_PTR(&c, val_out);

    key_out = 0; val_out = NULL;
    TEST_ASSERT_TRUE(pqueue_pop_min(&self, &key_out, &val_out));
    TEST_ASSERT_EQUAL_INT32(5, key_out);
    TEST_ASSERT_EQUAL_PTR(&a, val_out);

    TEST_ASSERT_TRUE(pqueue_is_empty(&self));
    TEST_ASSERT_EQUAL_size_t(0, pqueue_len(&self));
}

/* Bullet 4: filled to cap then push -> false (full). */
static void test_pqueue_push_when_full_returns_false(void)
{
    pqueue_node storage[4];
    pqueue self;

    pqueue_init(&self, storage, 4);

    TEST_ASSERT_TRUE(pqueue_push(&self, 10, &a));
    TEST_ASSERT_TRUE(pqueue_push(&self, 20, &b));
    TEST_ASSERT_TRUE(pqueue_push(&self, 30, &c));
    TEST_ASSERT_TRUE(pqueue_push(&self, 40, &a));
    TEST_ASSERT_EQUAL_size_t(4, pqueue_len(&self));

    /* cap reached -> push rejected, len unchanged */
    TEST_ASSERT_FALSE(pqueue_push(&self, 50, &b));
    TEST_ASSERT_EQUAL_size_t(4, pqueue_len(&self));
}

/* Bullet 5: empty heap: pop_min/peek_min -> false, key_out/val_out
 * untouched. */
static void test_pqueue_empty_pop_peek_false_outs_untouched(void)
{
    pqueue_node storage[8];
    pqueue self;
    int32_t key_out = 12345;
    void   *val_out = &c;

    pqueue_init(&self, storage, 8);
    TEST_ASSERT_TRUE(pqueue_is_empty(&self));

    TEST_ASSERT_FALSE(pqueue_peek_min(&self, &key_out, &val_out));
    /* outs untouched on empty */
    TEST_ASSERT_EQUAL_INT32(12345, key_out);
    TEST_ASSERT_EQUAL_PTR(&c, val_out);

    TEST_ASSERT_FALSE(pqueue_pop_min(&self, &key_out, &val_out));
    TEST_ASSERT_EQUAL_INT32(12345, key_out);
    TEST_ASSERT_EQUAL_PTR(&c, val_out);
}

/* Bullet 6: pop_min(self,NULL,NULL) still removes the min (NULL outs
 * tolerated). */
static void test_pqueue_pop_min_null_outs_still_removes(void)
{
    pqueue_node storage[8];
    pqueue self;
    int32_t key_out;
    void   *val_out;

    pqueue_init(&self, storage, 8);
    TEST_ASSERT_TRUE(pqueue_push(&self, 7, &a));
    TEST_ASSERT_TRUE(pqueue_push(&self, 2, &b));
    TEST_ASSERT_EQUAL_size_t(2, pqueue_len(&self));

    /* NULL outs tolerated; the min (key 2) is still removed */
    TEST_ASSERT_TRUE(pqueue_pop_min(&self, NULL, NULL));
    TEST_ASSERT_EQUAL_size_t(1, pqueue_len(&self));

    /* the remaining element is now the min (key 7, val a) */
    key_out = 0; val_out = NULL;
    TEST_ASSERT_TRUE(pqueue_peek_min(&self, &key_out, &val_out));
    TEST_ASSERT_EQUAL_INT32(7, key_out);
    TEST_ASSERT_EQUAL_PTR(&a, val_out);
}

/* Bullet 7: duplicate keys allowed: both entries pop out. */
static void test_pqueue_duplicate_keys_both_pop(void)
{
    pqueue_node storage[8];
    pqueue self;
    int32_t key_out;
    void   *val_out;

    pqueue_init(&self, storage, 8);
    TEST_ASSERT_TRUE(pqueue_push(&self, 4, &a));
    TEST_ASSERT_TRUE(pqueue_push(&self, 4, &b)); /* duplicate key */
    TEST_ASSERT_EQUAL_size_t(2, pqueue_len(&self));

    /* first pop: key 4 (one of the two values) */
    key_out = 0; val_out = NULL;
    TEST_ASSERT_TRUE(pqueue_pop_min(&self, &key_out, &val_out));
    TEST_ASSERT_EQUAL_INT32(4, key_out);
    TEST_ASSERT_TRUE(val_out == &a || val_out == &b);
    void *first = val_out;

    /* second pop: key 4 again, the other value */
    key_out = 0; val_out = NULL;
    TEST_ASSERT_TRUE(pqueue_pop_min(&self, &key_out, &val_out));
    TEST_ASSERT_EQUAL_INT32(4, key_out);
    TEST_ASSERT_TRUE(val_out == &a || val_out == &b);
    TEST_ASSERT_TRUE(val_out != first); /* both distinct entries came out */

    TEST_ASSERT_TRUE(pqueue_is_empty(&self));
}

/* Bullet 8: NULL self: pqueue_push(NULL,..) false, pqueue_pop_min(NULL,..)
 * false, pqueue_len(NULL) 0, pqueue_is_empty(NULL) true. */
static void test_pqueue_null_self_is_safe(void)
{
    int32_t key_out = 0;
    void   *val_out = NULL;

    TEST_ASSERT_FALSE(pqueue_push(NULL, 1, &a));
    TEST_ASSERT_FALSE(pqueue_pop_min(NULL, &key_out, &val_out));
    TEST_ASSERT_FALSE(pqueue_peek_min(NULL, &key_out, &val_out));
    TEST_ASSERT_EQUAL_size_t(0, pqueue_len(NULL));
    TEST_ASSERT_TRUE(pqueue_is_empty(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pqueue_init_empty);
    RUN_TEST(test_pqueue_peek_min_returns_min_without_removal);
    RUN_TEST(test_pqueue_pop_min_yields_ascending_keys);
    RUN_TEST(test_pqueue_push_when_full_returns_false);
    RUN_TEST(test_pqueue_empty_pop_peek_false_outs_untouched);
    RUN_TEST(test_pqueue_pop_min_null_outs_still_removes);
    RUN_TEST(test_pqueue_duplicate_keys_both_pop);
    RUN_TEST(test_pqueue_null_self_is_safe);
    return UNITY_END();
}
