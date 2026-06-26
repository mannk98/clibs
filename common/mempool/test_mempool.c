/* Unity host test for mempool (fixed-block allocator over caller storage).
 *
 * RED step: tests the frozen API contract only. The implementation
 * (mempool.c) does not exist yet, so this MUST fail to link
 * (undefined references to mempool_* symbols). The header mempool.h
 * carries the contract under test.
 *
 * No malloc: all storage is caller-provided, _Alignas(MEMPOOL_ALIGN)
 * uint8_t arrays sized with MEMPOOL_BYTES(block_size, block_count).
 */
#include "unity.h"
#include "mempool.h"

void setUp(void)    {}
void tearDown(void) {}

/* Bullet 1: storage=_Alignas(MEMPOOL_ALIGN) uint8_t[MEMPOOL_BYTES(sizeof(int),4)],
 * init(.,sizeof(int),4): capacity==4, free_count==4. */
static void test_mempool_init_sets_capacity_and_free_count(void)
{
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 4)];
    mempool self;

    mempool_init(&self, storage, sizeof(int), 4);

    TEST_ASSERT_EQUAL_size_t(4, mempool_capacity(&self));
    TEST_ASSERT_EQUAL_size_t(4, mempool_free_count(&self));
}

/* Bullet 2: Four allocs each non-NULL, free_count -> 0; fifth alloc
 * returns NULL (exhausted). */
static void test_mempool_alloc_exhausts_then_returns_null(void)
{
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 4)];
    mempool self;

    mempool_init(&self, storage, sizeof(int), 4);

    void *b0 = mempool_alloc(&self);
    void *b1 = mempool_alloc(&self);
    void *b2 = mempool_alloc(&self);
    void *b3 = mempool_alloc(&self);

    TEST_ASSERT_NOT_NULL(b0);
    TEST_ASSERT_NOT_NULL(b1);
    TEST_ASSERT_NOT_NULL(b2);
    TEST_ASSERT_NOT_NULL(b3);
    TEST_ASSERT_EQUAL_size_t(0, mempool_free_count(&self));

    TEST_ASSERT_NULL(mempool_alloc(&self)); /* exhausted */
}

/* Bullet 3: 3-block pool: two allocs return distinct non-NULL pointers;
 * *(int*)a=42 and *(int*)b=-7 read back correctly (aligned+usable). */
static void test_mempool_blocks_distinct_aligned_usable(void)
{
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 3)];
    mempool self;

    mempool_init(&self, storage, sizeof(int), 3);

    void *a = mempool_alloc(&self);
    void *b = mempool_alloc(&self);

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_TRUE(a != b); /* distinct */

    /* aligned to MEMPOOL_ALIGN */
    TEST_ASSERT_EQUAL_size_t(0, ((uintptr_t)a) % MEMPOOL_ALIGN);
    TEST_ASSERT_EQUAL_size_t(0, ((uintptr_t)b) % MEMPOOL_ALIGN);

    *(int *)a = 42;
    *(int *)b = -7;
    TEST_ASSERT_EQUAL_INT(42, *(int *)a); /* usable */
    TEST_ASSERT_EQUAL_INT(-7, *(int *)b);
}

/* Bullet 4: Full 2-block pool (free_count 0): free(a) -> free_count 1;
 * next alloc non-NULL (reused), free_count 0. */
static void test_mempool_free_reuses_block(void)
{
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 2)];
    mempool self;

    mempool_init(&self, storage, sizeof(int), 2);

    void *a = mempool_alloc(&self);
    void *b = mempool_alloc(&self);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_size_t(0, mempool_free_count(&self)); /* full */

    mempool_free(&self, a);
    TEST_ASSERT_EQUAL_size_t(1, mempool_free_count(&self));

    void *c = mempool_alloc(&self); /* reuse the returned block */
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL_size_t(0, mempool_free_count(&self));
}

/* Bullet 5: mempool_free(self, NULL) is a no-op (free_count unchanged). */
static void test_mempool_free_null_block_is_noop(void)
{
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 4)];
    mempool self;

    mempool_init(&self, storage, sizeof(int), 4);

    (void)mempool_alloc(&self); /* free_count -> 3 */
    size_t before = mempool_free_count(&self);

    mempool_free(&self, NULL); /* no-op */

    TEST_ASSERT_EQUAL_size_t(before, mempool_free_count(&self));
}

/* Bullet 6: NULL self: mempool_alloc(NULL)==NULL,
 * mempool_free(NULL,storage) no crash, mempool_capacity(NULL)==0,
 * mempool_free_count(NULL)==0. */
static void test_mempool_null_self_is_safe(void)
{
    _Alignas(MEMPOOL_ALIGN) uint8_t storage[MEMPOOL_BYTES(sizeof(int), 1)];

    TEST_ASSERT_NULL(mempool_alloc(NULL));

    mempool_free(NULL, storage); /* must not crash */

    TEST_ASSERT_EQUAL_size_t(0, mempool_capacity(NULL));
    TEST_ASSERT_EQUAL_size_t(0, mempool_free_count(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mempool_init_sets_capacity_and_free_count);
    RUN_TEST(test_mempool_alloc_exhausts_then_returns_null);
    RUN_TEST(test_mempool_blocks_distinct_aligned_usable);
    RUN_TEST(test_mempool_free_reuses_block);
    RUN_TEST(test_mempool_free_null_block_is_noop);
    RUN_TEST(test_mempool_null_self_is_safe);
    return UNITY_END();
}
