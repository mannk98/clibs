/* Unity host test for the `vec` lib (RED step of TDD).
 *
 * Tests ONLY the frozen API contract from the batch-2 design spec; the
 * production impl (vec.c) does not exist yet, so this is expected to FAIL to
 * link (undefined references to vec_*) — or to fail an assertion against a
 * stub. No production code is written here. One test per acceptance bullet.
 */
#include "unity.h"
#include "vec.h"

void setUp(void)    {}
void tearDown(void) {}

/* Bullet 1: int storage cap 4 — init -> len 0, cap 4, is_empty true, is_full false. */
static void test_vec_init_empty_state(void) {
    int storage[4];
    vec v;
    vec_init(&v, storage, 4, sizeof(int));
    TEST_ASSERT_EQUAL_size_t(0, vec_len(&v));
    TEST_ASSERT_EQUAL_size_t(4, vec_cap(&v));
    TEST_ASSERT_TRUE(vec_is_empty(&v));
    TEST_ASSERT_FALSE(vec_is_full(&v));
}

/* Bullet 2: push 1,2,3 -> len 3; get(0)=1, get(1)=2, get(2)=3; get(3) false. */
static void test_vec_push_get_in_order(void) {
    int storage[4];
    vec v;
    vec_init(&v, storage, 4, sizeof(int));

    int a = 1, b = 2, c = 3;
    TEST_ASSERT_TRUE(vec_push(&v, &a));
    TEST_ASSERT_TRUE(vec_push(&v, &b));
    TEST_ASSERT_TRUE(vec_push(&v, &c));
    TEST_ASSERT_EQUAL_size_t(3, vec_len(&v));

    int out = -1;
    TEST_ASSERT_TRUE(vec_get(&v, 0, &out));
    TEST_ASSERT_EQUAL_INT(1, out);
    TEST_ASSERT_TRUE(vec_get(&v, 1, &out));
    TEST_ASSERT_EQUAL_INT(2, out);
    TEST_ASSERT_TRUE(vec_get(&v, 2, &out));
    TEST_ASSERT_EQUAL_INT(3, out);

    out = 777;                                   /* sentinel: must stay untouched */
    TEST_ASSERT_FALSE(vec_get(&v, 3, &out));     /* index == len -> false */
    TEST_ASSERT_EQUAL_INT(777, out);             /* out untouched on failure */
}

/* Bullet 3: push 4 -> is_full true; push 5 -> false, len stays 4. */
static void test_vec_push_until_full(void) {
    int storage[4];
    vec v;
    vec_init(&v, storage, 4, sizeof(int));

    int vals[4] = {1, 2, 3, 4};
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(vec_push(&v, &vals[i]));
    }
    TEST_ASSERT_TRUE(vec_is_full(&v));
    TEST_ASSERT_EQUAL_size_t(4, vec_len(&v));

    int five = 5;
    TEST_ASSERT_FALSE(vec_push(&v, &five));      /* full -> reject */
    TEST_ASSERT_EQUAL_size_t(4, vec_len(&v));    /* len unchanged */
}

/* Bullet 4: set(1,99) -> get(1)=99; set(9,..) false (out of range). */
static void test_vec_set_and_out_of_range(void) {
    int storage[4];
    vec v;
    vec_init(&v, storage, 4, sizeof(int));

    int vals[3] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(vec_push(&v, &vals[i]));
    }

    int ninety_nine = 99;
    TEST_ASSERT_TRUE(vec_set(&v, 1, &ninety_nine));
    int out = -1;
    TEST_ASSERT_TRUE(vec_get(&v, 1, &out));
    TEST_ASSERT_EQUAL_INT(99, out);

    int x = 12345;
    TEST_ASSERT_FALSE(vec_set(&v, 9, &x));       /* i >= len -> false, no write */
    TEST_ASSERT_EQUAL_size_t(3, vec_len(&v));    /* len unchanged */
}

/* Bullet 5: pop -> out=4, len 3; pop down to empty -> is_empty true;
 *           pop on empty -> false, out untouched. */
static void test_vec_pop_down_to_empty(void) {
    int storage[4];
    vec v;
    vec_init(&v, storage, 4, sizeof(int));

    int vals[4] = {1, 2, 3, 4};
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(vec_push(&v, &vals[i]));
    }

    int out = -1;
    TEST_ASSERT_TRUE(vec_pop(&v, &out));         /* LIFO: last pushed (4) */
    TEST_ASSERT_EQUAL_INT(4, out);
    TEST_ASSERT_EQUAL_size_t(3, vec_len(&v));

    TEST_ASSERT_TRUE(vec_pop(&v, &out));         /* 3 */
    TEST_ASSERT_EQUAL_INT(3, out);
    TEST_ASSERT_TRUE(vec_pop(&v, &out));         /* 2 */
    TEST_ASSERT_EQUAL_INT(2, out);
    TEST_ASSERT_TRUE(vec_pop(&v, &out));         /* 1 */
    TEST_ASSERT_EQUAL_INT(1, out);

    TEST_ASSERT_TRUE(vec_is_empty(&v));
    TEST_ASSERT_EQUAL_size_t(0, vec_len(&v));

    out = 555;                                   /* sentinel: must stay untouched */
    TEST_ASSERT_FALSE(vec_pop(&v, &out));        /* empty -> false */
    TEST_ASSERT_EQUAL_INT(555, out);             /* out untouched on empty pop */
}

/* Bullet 6: clear -> len 0 (storage untouched). */
static void test_vec_clear(void) {
    int storage[4];
    vec v;
    vec_init(&v, storage, 4, sizeof(int));

    int vals[3] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(vec_push(&v, &vals[i]));
    }

    vec_clear(&v);
    TEST_ASSERT_EQUAL_size_t(0, vec_len(&v));
    TEST_ASSERT_TRUE(vec_is_empty(&v));

    /* storage untouched: raw bytes still hold the old values; re-pushing then
     * reading back the first slot must still observe a fresh write path. */
    int re = 42;
    TEST_ASSERT_TRUE(vec_push(&v, &re));
    int out = -1;
    TEST_ASSERT_TRUE(vec_get(&v, 0, &out));
    TEST_ASSERT_EQUAL_INT(42, out);
    TEST_ASSERT_EQUAL_size_t(1, vec_len(&v));
}

/* Bullet 7: value semantics — exactly elem_size bytes memcpy'd per push/pop/get/set.
 *           Use a multi-field struct element to prove a full-width copy (not just
 *           the first int). */
static void test_vec_value_semantics_elem_size(void) {
    typedef struct { int a; char b; long c; } item;
    item storage[3];
    vec v;
    vec_init(&v, storage, 3, sizeof(item));

    item in0 = { .a = 7,  .b = 'x', .c = 123456789L };
    item in1 = { .a = -9, .b = 'y', .c = -987654321L };
    TEST_ASSERT_TRUE(vec_push(&v, &in0));
    TEST_ASSERT_TRUE(vec_push(&v, &in1));

    item got = {0};
    TEST_ASSERT_TRUE(vec_get(&v, 0, &got));
    TEST_ASSERT_EQUAL_INT(7, got.a);
    TEST_ASSERT_EQUAL_INT('x', got.b);
    TEST_ASSERT_EQUAL_INT64(123456789L, got.c);

    /* set must copy the whole struct, not a prefix */
    item rep = { .a = 100, .b = 'z', .c = 555L };
    TEST_ASSERT_TRUE(vec_set(&v, 0, &rep));
    TEST_ASSERT_TRUE(vec_get(&v, 0, &got));
    TEST_ASSERT_EQUAL_INT(100, got.a);
    TEST_ASSERT_EQUAL_INT('z', got.b);
    TEST_ASSERT_EQUAL_INT64(555L, got.c);

    /* pop must copy the whole last struct */
    item popped = {0};
    TEST_ASSERT_TRUE(vec_pop(&v, &popped));
    TEST_ASSERT_EQUAL_INT(-9, popped.a);
    TEST_ASSERT_EQUAL_INT('y', popped.b);
    TEST_ASSERT_EQUAL_INT64(-987654321L, popped.c);
}

/* Bullet 8: NULL self — push/pop/get false, len 0, is_empty true. No crash. */
static void test_vec_null_self_safe(void) {
    int x = 1;
    int out = 333;                               /* sentinel for get failure */

    TEST_ASSERT_FALSE(vec_push(NULL, &x));
    TEST_ASSERT_FALSE(vec_pop(NULL, &out));
    TEST_ASSERT_FALSE(vec_get(NULL, 0, &out));
    TEST_ASSERT_EQUAL_INT(333, out);             /* out untouched on NULL self */

    TEST_ASSERT_FALSE(vec_set(NULL, 0, &x));     /* NULL-self-safe per OOP rules */
    vec_clear(NULL);                             /* must not crash */
    vec_init(NULL, &x, 1, sizeof(int));          /* must not crash */

    TEST_ASSERT_EQUAL_size_t(0, vec_len(NULL));
    TEST_ASSERT_EQUAL_size_t(0, vec_cap(NULL));
    TEST_ASSERT_TRUE(vec_is_empty(NULL));
    TEST_ASSERT_FALSE(vec_is_full(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_vec_init_empty_state);
    RUN_TEST(test_vec_push_get_in_order);
    RUN_TEST(test_vec_push_until_full);
    RUN_TEST(test_vec_set_and_out_of_range);
    RUN_TEST(test_vec_pop_down_to_empty);
    RUN_TEST(test_vec_clear);
    RUN_TEST(test_vec_value_semantics_elem_size);
    RUN_TEST(test_vec_null_self_safe);
    return UNITY_END();
}
