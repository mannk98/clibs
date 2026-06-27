#include "unity.h"

/* Test only the frozen API contract — tlv.c does not exist yet (RED step).
 * Contract (from batch-4 design spec, CLIBS_TLV_H):
 *
 *   typedef struct { uint8_t *buf; size_t cap; size_t len; } tlv_writer;
 *   void   tlv_writer_init(tlv_writer *self, void *buf, size_t cap);
 *   bool   tlv_write(tlv_writer *self, uint8_t type, const void *value, uint8_t vlen);
 *   size_t tlv_writer_len(const tlv_writer *self);
 *
 *   typedef struct { const uint8_t *buf; size_t len; size_t pos; } tlv_reader;
 *   void   tlv_reader_init(tlv_reader *self, const void *buf, size_t len);
 *   bool   tlv_next(tlv_reader *self, uint8_t *type, const uint8_t **value, uint8_t *vlen);
 *   bool   tlv_find(const tlv_reader *self, uint8_t type, const uint8_t **value, uint8_t *vlen);
 *
 * Record layout: [type:1][vlen:1][value:vlen]. tlv_write needs 2+vlen free bytes
 * (false otherwise, buffer/len unchanged). tlv_next reads the record at pos
 * (needs pos+2 <= len and pos+2+vlen <= len, else false=truncated), advances pos.
 * tlv_find scans independently from the start and takes const (pos unchanged).
 * NULL self is safe; out params for next/find are required non-NULL. */
#include "tlv.h"

void setUp(void) {}
void tearDown(void) {}

/* Bullet 1: tlv_writer over buf[64], init then tlv_writer_len == 0. */
static void test_tlv_writer_init_len_zero(void) {
    uint8_t buf[64];
    tlv_writer w;

    tlv_writer_init(&w, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_size_t(0, tlv_writer_len(&w));
}

/* Bullet 2: tlv_write(1,"ab",2) -> true, len 4; then tlv_write(2,&x,1) -> true,
 * len 7 (records [type][vlen][value]). */
static void test_tlv_write_two_records_advances_len(void) {
    uint8_t buf[64];
    tlv_writer w;
    const uint8_t x = 0x5A;

    tlv_writer_init(&w, buf, sizeof(buf));

    TEST_ASSERT_TRUE(tlv_write(&w, 1, "ab", 2));
    TEST_ASSERT_EQUAL_size_t(4, tlv_writer_len(&w));

    TEST_ASSERT_TRUE(tlv_write(&w, 2, &x, 1));
    TEST_ASSERT_EQUAL_size_t(7, tlv_writer_len(&w));

    /* Records were written verbatim: [type][vlen][value...]. */
    const uint8_t expected[] = {1, 2, 'a', 'b', 2, 1, 0x5A};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, buf, sizeof(expected));
}

/* Bullet 3: tiny buffer: tlv_write of a record needing 2+vlen bytes that
 * doesn't fit -> false, buffer/len unchanged. */
static void test_tlv_write_no_room_returns_false_unchanged(void) {
    uint8_t buf[3] = {0xEE, 0xEE, 0xEE};   /* cap 3; record needs 2+2 = 4 bytes */
    tlv_writer w;

    tlv_writer_init(&w, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_size_t(0, tlv_writer_len(&w));

    /* type 7, value "ab" (vlen 2) needs 4 bytes, only 3 available -> false. */
    TEST_ASSERT_FALSE(tlv_write(&w, 7, "ab", 2));

    /* len unchanged. */
    TEST_ASSERT_EQUAL_size_t(0, tlv_writer_len(&w));
    /* buffer bytes unchanged (no partial write). */
    const uint8_t untouched[] = {0xEE, 0xEE, 0xEE};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(untouched, buf, sizeof(untouched));
}

/* Bullet 4: tlv_reader over the built buffer: tlv_next -> type 1, vlen 2,
 * value 'a','b'; tlv_next -> type 2, vlen 1; tlv_next -> false (end). */
static void test_tlv_next_iterates_then_end(void) {
    /* Two records: [1][2]['a','b']  [2][1][0x5A] */
    const uint8_t data[] = {1, 2, 'a', 'b', 2, 1, 0x5A};
    tlv_reader r;
    uint8_t type;
    const uint8_t *value;
    uint8_t vlen;

    tlv_reader_init(&r, data, sizeof(data));

    /* First record. */
    TEST_ASSERT_TRUE(tlv_next(&r, &type, &value, &vlen));
    TEST_ASSERT_EQUAL_UINT8(1, type);
    TEST_ASSERT_EQUAL_UINT8(2, vlen);
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_UINT8('a', value[0]);
    TEST_ASSERT_EQUAL_UINT8('b', value[1]);

    /* Second record. */
    TEST_ASSERT_TRUE(tlv_next(&r, &type, &value, &vlen));
    TEST_ASSERT_EQUAL_UINT8(2, type);
    TEST_ASSERT_EQUAL_UINT8(1, vlen);
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_UINT8(0x5A, value[0]);

    /* End. */
    TEST_ASSERT_FALSE(tlv_next(&r, &type, &value, &vlen));
}

/* Bullet 5: tlv_find(2) -> true with value/vlen of type-2 record, reader pos
 * unchanged (const, independent scan). */
static void test_tlv_find_present_pos_unchanged(void) {
    const uint8_t data[] = {1, 2, 'a', 'b', 2, 1, 0x5A};
    tlv_reader r;
    uint8_t type;
    const uint8_t *value;
    uint8_t vlen;

    tlv_reader_init(&r, data, sizeof(data));

    /* find(2) returns the type-2 record. */
    TEST_ASSERT_TRUE(tlv_find(&r, 2, &value, &vlen));
    TEST_ASSERT_EQUAL_UINT8(1, vlen);
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_UINT8(0x5A, value[0]);

    /* find is const / independent: reader pos is untouched, so a subsequent
     * tlv_next still returns the FIRST record (type 1). */
    TEST_ASSERT_TRUE(tlv_next(&r, &type, &value, &vlen));
    TEST_ASSERT_EQUAL_UINT8(1, type);
    TEST_ASSERT_EQUAL_UINT8(2, vlen);
}

/* Bullet 6: tlv_find(9) absent -> false. */
static void test_tlv_find_absent_returns_false(void) {
    const uint8_t data[] = {1, 2, 'a', 'b', 2, 1, 0x5A};
    tlv_reader r;
    const uint8_t *value = (const uint8_t *)1;  /* sentinel; must stay untouched-or-ignored */
    uint8_t vlen = 0xFF;

    tlv_reader_init(&r, data, sizeof(data));
    TEST_ASSERT_FALSE(tlv_find(&r, 9, &value, &vlen));
}

/* Bullet 7: malformed/truncated: reader over {0x01,0x05,0xAA} (claims vlen 5,
 * 1 byte present) -> tlv_next false. */
static void test_tlv_next_truncated_returns_false(void) {
    const uint8_t data[] = {0x01, 0x05, 0xAA};  /* vlen claims 5, only 1 value byte */
    tlv_reader r;
    uint8_t type;
    const uint8_t *value;
    uint8_t vlen;

    tlv_reader_init(&r, data, sizeof(data));
    TEST_ASSERT_FALSE(tlv_next(&r, &type, &value, &vlen));
}

/* Bullet 8: empty: writer len 0; reader over zero-length buffer -> tlv_next false. */
static void test_tlv_empty_writer_and_reader(void) {
    uint8_t buf[8];
    tlv_writer w;
    tlv_reader r;
    uint8_t type;
    const uint8_t *value;
    uint8_t vlen;

    /* Empty writer. */
    tlv_writer_init(&w, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_size_t(0, tlv_writer_len(&w));

    /* Reader over a zero-length buffer -> immediately at end. */
    tlv_reader_init(&r, buf, 0);
    TEST_ASSERT_FALSE(tlv_next(&r, &type, &value, &vlen));
}

/* Bullet 9: NULL self: tlv_write(NULL,..) false; tlv_next(NULL,..) false;
 * tlv_writer_len(NULL) 0; tlv_find(NULL,..) false; init(NULL,..) no-op, no crash. */
static void test_tlv_null_self_safe(void) {
    uint8_t type;
    const uint8_t *value;
    uint8_t vlen;
    const uint8_t x = 0x01;

    /* Constructors on NULL self: no-op, no crash. */
    tlv_writer_init(NULL, (void *)0, 0);
    tlv_reader_init(NULL, (const void *)0, 0);

    /* Methods on NULL self return the defined failure value. */
    TEST_ASSERT_FALSE(tlv_write(NULL, 1, &x, 1));
    TEST_ASSERT_EQUAL_size_t(0, tlv_writer_len(NULL));
    TEST_ASSERT_FALSE(tlv_next(NULL, &type, &value, &vlen));
    TEST_ASSERT_FALSE(tlv_find(NULL, 1, &value, &vlen));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tlv_writer_init_len_zero);
    RUN_TEST(test_tlv_write_two_records_advances_len);
    RUN_TEST(test_tlv_write_no_room_returns_false_unchanged);
    RUN_TEST(test_tlv_next_iterates_then_end);
    RUN_TEST(test_tlv_find_present_pos_unchanged);
    RUN_TEST(test_tlv_find_absent_returns_false);
    RUN_TEST(test_tlv_next_truncated_returns_false);
    RUN_TEST(test_tlv_empty_writer_and_reader);
    RUN_TEST(test_tlv_null_self_safe);
    return UNITY_END();
}
