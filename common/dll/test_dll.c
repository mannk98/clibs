#include "unity.h"
#include "dll.h"

void setUp(void) {}
void tearDown(void) {}

/* values stored are pointers to these ints */
static int A = 1, B = 2, C = 3;

static int match_int(const void *stored, const void *key) {
    return *(const int *)stored - *(const int *)key;
}

static void test_init_empty(void) {
    dll_node pool[4];
    dll_list l;
    dll_init(&l, pool, 4);
    TEST_ASSERT_TRUE(dll_is_empty(&l));
    TEST_ASSERT_FALSE(dll_is_full(&l));
    TEST_ASSERT_EQUAL_UINT16(0, dll_count(&l));
}

static void test_push_tail_pop_head_fifo(void) {
    dll_node pool[4];
    dll_list l;
    dll_init(&l, pool, 4);
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_push_tail(&l, &A));
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_push_tail(&l, &B));
    void *out;
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_pop_head(&l, &out));
    TEST_ASSERT_EQUAL_PTR(&A, out);
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_pop_head(&l, &out));
    TEST_ASSERT_EQUAL_PTR(&B, out);
    TEST_ASSERT_EQUAL_INT(DLL_EMPTY, dll_pop_head(&l, &out));
}

static void test_push_head_and_pop_tail(void) {
    dll_node pool[4];
    dll_list l;
    dll_init(&l, pool, 4);
    dll_push_head(&l, &A);          /* [A]      */
    dll_push_head(&l, &B);          /* [B, A]   */
    void *out;
    dll_pop_tail(&l, &out); TEST_ASSERT_EQUAL_PTR(&A, out);
    dll_pop_tail(&l, &out); TEST_ASSERT_EQUAL_PTR(&B, out);
    TEST_ASSERT_TRUE(dll_is_empty(&l));
}

static void test_full_capacity(void) {
    dll_node pool[2];
    dll_list l;
    dll_init(&l, pool, 2);
    TEST_ASSERT_EQUAL_INT(DLL_OK,   dll_push_tail(&l, &A));
    TEST_ASSERT_EQUAL_INT(DLL_OK,   dll_push_tail(&l, &B));
    TEST_ASSERT_TRUE(dll_is_full(&l));
    TEST_ASSERT_EQUAL_INT(DLL_FULL, dll_push_tail(&l, &C));
}

static void test_pool_reuse_via_free_list(void) {
    dll_node pool[2];
    dll_list l;
    dll_init(&l, pool, 2);
    void *out;
    dll_push_tail(&l, &A);
    dll_pop_head(&l, &out);          /* node returns to free-list */
    dll_push_tail(&l, &B);           /* must succeed by reusing it */
    dll_push_tail(&l, &C);
    TEST_ASSERT_TRUE(dll_is_full(&l));
    dll_pop_head(&l, &out); TEST_ASSERT_EQUAL_PTR(&B, out);
    dll_pop_head(&l, &out); TEST_ASSERT_EQUAL_PTR(&C, out);
}

static void test_find_hit_and_miss(void) {
    dll_node pool[4];
    dll_list l;
    dll_init(&l, pool, 4);
    dll_push_tail(&l, &A);
    dll_push_tail(&l, &B);
    dll_node *found = NULL;
    int key = 2;
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_find(&l, &key, match_int, &found));
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(&B, found->value);
    int missing = 99;
    TEST_ASSERT_EQUAL_INT(DLL_NOT_FOUND, dll_find(&l, &missing, match_int, &found));
    TEST_ASSERT_NULL(found);
}

static void test_clear(void) {
    dll_node pool[3];
    dll_list l;
    dll_init(&l, pool, 3);
    dll_push_tail(&l, &A);
    dll_push_tail(&l, &B);
    dll_clear(&l);
    TEST_ASSERT_TRUE(dll_is_empty(&l));
    /* after clear all nodes are free again */
    TEST_ASSERT_EQUAL_INT(DLL_OK, dll_push_tail(&l, &C));
    TEST_ASSERT_EQUAL_UINT16(1, dll_count(&l));
}

static void test_invalid_args(void) {
    TEST_ASSERT_EQUAL_INT(DLL_INVALID, dll_push_tail(NULL, &A));
    dll_node pool[1];
    dll_list l;
    dll_init(&l, pool, 1);
    TEST_ASSERT_EQUAL_INT(DLL_INVALID, dll_find(&l, &A, NULL, NULL));
}

static void test_init_null_pool_is_safe(void) {
    dll_list l;
    dll_init(&l, NULL, 4);                       /* bad config: NULL pool */
    TEST_ASSERT_TRUE(dll_is_empty(&l));
    TEST_ASSERT_TRUE(dll_is_full(&l));           /* cap forced to 0 */
    TEST_ASSERT_EQUAL_INT(DLL_FULL, dll_push_tail(&l, &A)); /* no NULL deref */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_empty);
    RUN_TEST(test_push_tail_pop_head_fifo);
    RUN_TEST(test_push_head_and_pop_tail);
    RUN_TEST(test_full_capacity);
    RUN_TEST(test_pool_reuse_via_free_list);
    RUN_TEST(test_find_hit_and_miss);
    RUN_TEST(test_clear);
    RUN_TEST(test_invalid_args);
    RUN_TEST(test_init_null_pool_is_safe);
    return UNITY_END();
}
