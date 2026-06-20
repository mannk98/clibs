#include "unity.h"
#include "ringbuf.h"

void setUp(void) {}
void tearDown(void) {}

static void test_fifo_order_bytes(void) {
    uint8_t store[4];
    ringbuf rb;
    ringbuf_init(&rb, store, 4, sizeof(uint8_t), false);
    TEST_ASSERT_TRUE(ringbuf_is_empty(&rb));

    uint8_t in;
    in = 10; TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put(&rb, &in));
    in = 20; TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_put(&rb, &in));
    TEST_ASSERT_EQUAL_UINT16(2, ringbuf_count(&rb));

    uint8_t out;
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, &out));
    TEST_ASSERT_EQUAL_UINT8(10, out);              /* oldest first */
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, &out));
    TEST_ASSERT_EQUAL_UINT8(20, out);
    TEST_ASSERT_EQUAL_INT(RB_EMPTY, ringbuf_get(&rb, &out));
}

static void test_full_reject_when_no_overwrite(void) {
    uint8_t store[2];
    ringbuf rb;
    ringbuf_init(&rb, store, 2, 1, false);
    uint8_t v = 1;
    ringbuf_put(&rb, &v);
    ringbuf_put(&rb, &v);
    TEST_ASSERT_TRUE(ringbuf_is_full(&rb));
    TEST_ASSERT_EQUAL_INT(RB_FULL, ringbuf_put(&rb, &v));
    TEST_ASSERT_EQUAL_UINT16(2, ringbuf_count(&rb));
}

static void test_overwrite_evicts_oldest(void) {
    uint8_t store[3];
    ringbuf rb;
    ringbuf_init(&rb, store, 3, 1, true);
    for (uint8_t i = 1; i <= 3; i++) ringbuf_put(&rb, &i);
    uint8_t four = 4;
    TEST_ASSERT_EQUAL_INT(RB_OVERWRITE, ringbuf_put(&rb, &four)); /* drops 1 */
    uint8_t out;
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(2, out);      /* 1 was evicted */
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(3, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(4, out);
}

static void test_wraparound(void) {
    uint8_t store[3];
    ringbuf rb;
    ringbuf_init(&rb, store, 3, 1, false);
    uint8_t v, out;
    v = 1; ringbuf_put(&rb, &v);
    v = 2; ringbuf_put(&rb, &v);
    ringbuf_get(&rb, &out);            /* drop 1, head advances */
    v = 3; ringbuf_put(&rb, &v);
    v = 4; ringbuf_put(&rb, &v);       /* wraps into freed slot */
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(2, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(3, out);
    ringbuf_get(&rb, &out); TEST_ASSERT_EQUAL_UINT8(4, out);
}

static void test_peek_does_not_remove(void) {
    uint8_t store[2];
    ringbuf rb;
    ringbuf_init(&rb, store, 2, 1, false);
    uint8_t v = 99; ringbuf_put(&rb, &v);
    uint8_t out;
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_peek(&rb, &out));
    TEST_ASSERT_EQUAL_UINT8(99, out);
    TEST_ASSERT_EQUAL_UINT16(1, ringbuf_count(&rb)); /* still there */
}

static void test_multibyte_elements(void) {
    uint32_t store[2];
    ringbuf rb;
    ringbuf_init(&rb, store, 2, sizeof(uint32_t), false);
    uint32_t in = 0xDEADBEEF, out = 0;
    ringbuf_put(&rb, &in);
    ringbuf_get(&rb, &out);
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, out);
}

static void test_invalid_args(void) {
    ringbuf rb;
    uint8_t store[1];
    ringbuf_init(&rb, store, 1, 1, false);
    TEST_ASSERT_EQUAL_INT(RB_INVALID, ringbuf_put(&rb, NULL));
    TEST_ASSERT_EQUAL_INT(RB_INVALID, ringbuf_put(NULL, store));
}

static void test_get_null_out_drops(void) {
    uint8_t store[2];
    ringbuf rb;
    uint8_t v = 42;
    ringbuf_init(&rb, store, 2, 1, false);
    ringbuf_put(&rb, &v);
    TEST_ASSERT_EQUAL_INT(RB_OK, ringbuf_get(&rb, NULL)); /* drop without copy */
    TEST_ASSERT_TRUE(ringbuf_is_empty(&rb));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fifo_order_bytes);
    RUN_TEST(test_full_reject_when_no_overwrite);
    RUN_TEST(test_overwrite_evicts_oldest);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_peek_does_not_remove);
    RUN_TEST(test_multibyte_elements);
    RUN_TEST(test_invalid_args);
    RUN_TEST(test_get_null_out_drops);
    return UNITY_END();
}
