#include "unity.h"
#include "ringbuff.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_fifo_order(void) {
    uint8_t store[4]; ringbuf rb;
    ringbuf_init(&rb, store, 4, false);
    TEST_ASSERT_TRUE(ringbuf_is_empty(&rb));
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put(&rb, 10));
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put(&rb, 20));
    uint8_t out;
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, &out)); TEST_ASSERT_EQUAL_UINT8(10, out);
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, &out)); TEST_ASSERT_EQUAL_UINT8(20, out);
    TEST_ASSERT_EQUAL_INT(RB_EMPTY, ringbuf_get(&rb, &out));
}

static void test_full_reject_no_infinite_loop(void) {
    /* regression: old ringBuff_put busy-waited forever when full. */
    uint8_t store[2]; ringbuf rb;
    ringbuf_init(&rb, store, 2, false);
    ringbuf_put(&rb, 1); ringbuf_put(&rb, 2);
    TEST_ASSERT_TRUE(ringbuf_is_full(&rb));
    TEST_ASSERT_EQUAL_INT(RB_FULL, ringbuf_put(&rb, 3)); /* returns, does NOT hang */
    TEST_ASSERT_EQUAL_UINT16(2, ringbuf_count(&rb));
}

static void test_overwrite_evicts_oldest(void) {
    uint8_t store[3]; ringbuf rb;
    ringbuf_init(&rb, store, 3, true);
    for (uint8_t i = 1; i <= 3; i++) ringbuf_put(&rb, i);
    TEST_ASSERT_EQUAL_INT(RB_OVERWRITE, ringbuf_put(&rb, 4)); /* drops 1 */
    uint8_t out;
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(2, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(3, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(4, out);
}

static void test_wraparound(void) {
    uint8_t store[3]; ringbuf rb;
    ringbuf_init(&rb, store, 3, false);
    uint8_t out;
    ringbuf_put(&rb, 1); ringbuf_put(&rb, 2);
    ringbuf_get(&rb, &out);          /* drop 1 */
    ringbuf_put(&rb, 3); ringbuf_put(&rb, 4);   /* wraps */
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(2, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(3, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(4, out);
}

static void test_peek(void) {
    uint8_t store[2]; ringbuf rb;
    ringbuf_init(&rb, store, 2, false);
    ringbuf_put(&rb, 99);
    uint8_t out;
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_peek(&rb, &out));
    TEST_ASSERT_EQUAL_UINT8(99, out);
    TEST_ASSERT_EQUAL_UINT16(1, ringbuf_count(&rb));
}

static void test_put_string(void) {
    uint8_t store[8]; ringbuf rb;
    ringbuf_init(&rb, store, 8, false);
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put_string(&rb, "abc"));
    TEST_ASSERT_EQUAL_UINT16(3, ringbuf_count(&rb));
    uint8_t out;
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('a', out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('b', out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('c', out);
}

static void test_get_bytes_returns_count(void) {
    /* regression: old getBytes had a wrong success condition. */
    uint8_t store[8]; ringbuf rb;
    ringbuf_init(&rb, store, 8, false);
    for (uint8_t i = 1; i <= 5; i++) ringbuf_put(&rb, i);
    uint8_t out[10];
    TEST_ASSERT_EQUAL_UINT16(3, ringbuf_get_bytes(&rb, out, 3)); /* copies 3 */
    TEST_ASSERT_EQUAL_UINT8(1, out[0]);
    TEST_ASSERT_EQUAL_UINT8(3, out[2]);
    TEST_ASSERT_EQUAL_UINT16(2, ringbuf_get_bytes(&rb, out, 10)); /* only 2 left */
    TEST_ASSERT_EQUAL_UINT8(4, out[0]);
    TEST_ASSERT_EQUAL_UINT8(5, out[1]);
}

static void test_invalid_args(void) {
    ringbuf rb; uint8_t store[1];
    ringbuf_init(&rb, store, 1, false);
    TEST_ASSERT_EQUAL_INT(RB_INVALID, ringbuf_put(NULL, 1));
    ringbuf bad; ringbuf_init(&bad, NULL, 4, false);  /* bad init -> safe state */
    TEST_ASSERT_TRUE(ringbuf_is_empty(&bad));
    TEST_ASSERT_EQUAL_INT(RB_INVALID, ringbuf_put(&bad, 1));
}

static void test_put_string_overflow_reject(void) {
    uint8_t store[3]; ringbuf rb;
    ringbuf_init(&rb, store, 3, false);
    TEST_ASSERT_EQUAL_INT(RB_FULL, ringbuf_put_string(&rb, "abcde")); /* fills abc, then rejects */
    TEST_ASSERT_EQUAL_UINT16(3, ringbuf_count(&rb));
    uint8_t out;
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('a', out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('b', out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('c', out);
}
static void test_put_string_overflow_overwrite(void) {
    uint8_t store[3]; ringbuf rb;
    ringbuf_init(&rb, store, 3, true);
    TEST_ASSERT_EQUAL_INT(RB_OVERWRITE, ringbuf_put_string(&rb, "abcde")); /* keeps last 3: cde */
    uint8_t out;
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('c', out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('d', out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8('e', out);
}
static void test_get_bytes_max_zero(void) {
    uint8_t store[2]; ringbuf rb;
    ringbuf_init(&rb, store, 2, false);
    ringbuf_put(&rb, 7);
    uint8_t out[2] = { 0xAA, 0xAA };
    TEST_ASSERT_EQUAL_UINT16(0, ringbuf_get_bytes(&rb, out, 0)); /* copies nothing */
    TEST_ASSERT_EQUAL_UINT8(0xAA, out[0]);
    TEST_ASSERT_EQUAL_UINT16(1, ringbuf_count(&rb));            /* unchanged */
}
static void test_get_null_drop_and_peek_empty(void) {
    uint8_t store[2]; ringbuf rb;
    ringbuf_init(&rb, store, 2, false);
    ringbuf_put(&rb, 5);
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, NULL)); /* drop without copy */
    TEST_ASSERT_TRUE(ringbuf_is_empty(&rb));
    uint8_t out;
    TEST_ASSERT_EQUAL_INT(RB_EMPTY, ringbuf_peek(&rb, &out)); /* peek empty */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fifo_order);
    RUN_TEST(test_full_reject_no_infinite_loop);
    RUN_TEST(test_overwrite_evicts_oldest);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_peek);
    RUN_TEST(test_put_string);
    RUN_TEST(test_get_bytes_returns_count);
    RUN_TEST(test_invalid_args);
    RUN_TEST(test_put_string_overflow_reject);
    RUN_TEST(test_put_string_overflow_overwrite);
    RUN_TEST(test_get_bytes_max_zero);
    RUN_TEST(test_get_null_drop_and_peek_empty);
    return UNITY_END();
}
