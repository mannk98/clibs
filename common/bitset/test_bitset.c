/* Unity host test for the `bitset` lib (RED step of TDD).
 *
 * Tests ONLY the frozen API contract from the batch-1 design spec; the
 * production header/impl do not exist yet, so this is expected to FAIL to
 * link (undefined references to bitset_*) or to fail an assertion against
 * a stub. No production code is written here.
 */
#include "unity.h"
#include "bitset.h"

void setUp(void)    {}
void tearDown(void) {}

/* Bullet 1: Given a 64-bit bitset after init, count is 0. */
static void test_bitset_init_count_zero(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs;
    bitset_init(&bs, words, 64);
    TEST_ASSERT_EQUAL_size_t(0, bitset_count(&bs));
}

/* Bullet 2: set(0) and set(63) -> test(0)/test(63) true, test(1) false, count 2. */
static void test_bitset_set_test_count(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs;
    bitset_init(&bs, words, 64);
    bitset_set(&bs, 0);
    bitset_set(&bs, 63);
    TEST_ASSERT_TRUE(bitset_test(&bs, 0));
    TEST_ASSERT_TRUE(bitset_test(&bs, 63));
    TEST_ASSERT_FALSE(bitset_test(&bs, 1));
    TEST_ASSERT_EQUAL_size_t(2, bitset_count(&bs));
}

/* Bullet 3: clear(0) -> test(0) false and count 1. */
static void test_bitset_clear(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs;
    bitset_init(&bs, words, 64);
    bitset_set(&bs, 0);
    bitset_set(&bs, 63);
    bitset_clear(&bs, 0);
    TEST_ASSERT_FALSE(bitset_test(&bs, 0));
    TEST_ASSERT_EQUAL_size_t(1, bitset_count(&bs));
}

/* Bullet 4: with bit 63 set, toggle(1) -> true, count 2; toggle(1) -> false, count 1. */
static void test_bitset_toggle(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs;
    bitset_init(&bs, words, 64);
    bitset_set(&bs, 63);
    bitset_toggle(&bs, 1);
    TEST_ASSERT_TRUE(bitset_test(&bs, 1));
    TEST_ASSERT_EQUAL_size_t(2, bitset_count(&bs));
    bitset_toggle(&bs, 1);
    TEST_ASSERT_FALSE(bitset_test(&bs, 1));
    TEST_ASSERT_EQUAL_size_t(1, bitset_count(&bs));
}

/* Bullet 5: empty find_first_set false (*out unchanged); after set(5),set(40)
 *           find_first_set true and *out == 5. */
static void test_bitset_find_first_set(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs;
    bitset_init(&bs, words, 64);

    size_t out = 777;                         /* sentinel; must stay untouched */
    TEST_ASSERT_FALSE(bitset_find_first_set(&bs, &out));
    TEST_ASSERT_EQUAL_size_t(777, out);       /* *out unchanged when none set */

    bitset_set(&bs, 5);
    bitset_set(&bs, 40);
    out = 777;
    TEST_ASSERT_TRUE(bitset_find_first_set(&bs, &out));
    TEST_ASSERT_EQUAL_size_t(5, out);         /* lowest set index */
}

/* Bullet 6: 64-bit set_all -> count 64; clear_all -> count 0. */
static void test_bitset_set_all_clear_all_64(void) {
    uint32_t words[BITSET_WORDS(64)];
    bitset bs;
    bitset_init(&bs, words, 64);
    bitset_set_all(&bs);
    TEST_ASSERT_EQUAL_size_t(64, bitset_count(&bs));
    bitset_clear_all(&bs);
    TEST_ASSERT_EQUAL_size_t(0, bitset_count(&bs));
}

/* Bullet 7: nbits=10 (1 word) set_all -> count 10, test(9) true, test(10) false
 *           (partial-word mask). */
static void test_bitset_set_all_partial_word(void) {
    uint32_t words[BITSET_WORDS(10)];
    bitset bs;
    bitset_init(&bs, words, 10);
    bitset_set_all(&bs);
    TEST_ASSERT_EQUAL_size_t(10, bitset_count(&bs));
    TEST_ASSERT_TRUE(bitset_test(&bs, 9));
    TEST_ASSERT_FALSE(bitset_test(&bs, 10));  /* beyond nbits: never set */
}

/* Bullet 8: 8-bit bitset: set(100) no-op, test(100) false, count stays 0
 *           (out-of-range guard). */
static void test_bitset_out_of_range_guard(void) {
    uint32_t words[BITSET_WORDS(8)];
    bitset bs;
    bitset_init(&bs, words, 8);
    bitset_set(&bs, 100);                     /* out of range -> no-op */
    TEST_ASSERT_FALSE(bitset_test(&bs, 100)); /* out of range -> false */
    TEST_ASSERT_EQUAL_size_t(0, bitset_count(&bs));
}

/* Bullet 9: NULL self: bitset_set(NULL,0) no crash, bitset_test(NULL,0) false,
 *           bitset_count(NULL) 0. */
static void test_bitset_null_self_safe(void) {
    bitset_set(NULL, 0);                       /* must not crash */
    bitset_clear(NULL, 0);                     /* must not crash */
    bitset_toggle(NULL, 0);                    /* must not crash */
    bitset_set_all(NULL);                      /* must not crash */
    bitset_clear_all(NULL);                    /* must not crash */
    TEST_ASSERT_FALSE(bitset_test(NULL, 0));
    TEST_ASSERT_EQUAL_size_t(0, bitset_count(NULL));

    size_t out = 555;
    TEST_ASSERT_FALSE(bitset_find_first_set(NULL, &out));
    TEST_ASSERT_EQUAL_size_t(555, out);       /* *out unchanged on NULL self */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_bitset_init_count_zero);
    RUN_TEST(test_bitset_set_test_count);
    RUN_TEST(test_bitset_clear);
    RUN_TEST(test_bitset_toggle);
    RUN_TEST(test_bitset_find_first_set);
    RUN_TEST(test_bitset_set_all_clear_all_64);
    RUN_TEST(test_bitset_set_all_partial_word);
    RUN_TEST(test_bitset_out_of_range_guard);
    RUN_TEST(test_bitset_null_self_safe);
    return UNITY_END();
}
