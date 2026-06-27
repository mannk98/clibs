#include "unity.h"

/* Test only the frozen API contract — slip.c does not exist yet (RED step).
 * Contract (from batch-4 design spec, CLIBS_SLIP_H):
 *   size_t slip_encode(const void *data, size_t len, uint8_t *out, size_t out_size);
 *   size_t slip_decode(const void *slip, size_t len, uint8_t *out, size_t out_size);
 * RFC 1055: END=0xC0, ESC=0xDB, ESC_END=0xDC, ESC_ESC=0xDD.
 * encode emits the escaped payload PLUS a trailing END; decode unescapes up to
 * the first unescaped END. Both return the produced length, or 0 on
 * NULL / len 0 / out too small / malformed. On error nothing is written. */
#include "slip.h"

void setUp(void) {}
void tearDown(void) {}

/* Bullet 1: encode({0x01,0xC0,0x02},3) -> {0x01,0xDB,0xDC,0x02,0xC0}, returns 5
 * (END escaped to ESC ESC_END + trailing END). */
static void test_slip_encode_escapes_end_byte(void) {
    const uint8_t in[]  = {0x01, 0xC0, 0x02};
    const uint8_t exp[] = {0x01, 0xDB, 0xDC, 0x02, 0xC0};
    uint8_t out[16];

    size_t n = slip_encode(in, sizeof(in), out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

/* Bullet 2: encode({0x01,0xDB,0x02},3) -> {0x01,0xDB,0xDD,0x02,0xC0}, returns 5
 * (ESC escaped to ESC ESC_ESC + trailing END). */
static void test_slip_encode_escapes_esc_byte(void) {
    const uint8_t in[]  = {0x01, 0xDB, 0x02};
    const uint8_t exp[] = {0x01, 0xDB, 0xDD, 0x02, 0xC0};
    uint8_t out[16];

    size_t n = slip_encode(in, sizeof(in), out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

/* Bullet 3: encode({0x01,0x02},2) -> {0x01,0x02,0xC0}, returns 3
 * (verbatim + trailing END). */
static void test_slip_encode_verbatim_with_trailing_end(void) {
    const uint8_t in[]  = {0x01, 0x02};
    const uint8_t exp[] = {0x01, 0x02, 0xC0};
    uint8_t out[16];

    size_t n = slip_encode(in, sizeof(in), out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(3, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

/* Bullet 4: decode({0x01,0xDB,0xDC,0x02,0xC0},5) -> {0x01,0xC0,0x02}, returns 3. */
static void test_slip_decode_unescapes_frame(void) {
    const uint8_t in[]  = {0x01, 0xDB, 0xDC, 0x02, 0xC0};
    const uint8_t exp[] = {0x01, 0xC0, 0x02};
    uint8_t out[16];

    size_t n = slip_decode(in, sizeof(in), out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(3, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

/* Bullet 5: round-trip decode(encode(x)) == x for the above vectors plus a
 * buffer containing several END/ESC bytes (heavy stuffing). */
static void test_slip_round_trip(void) {
    const uint8_t v1[] = {0x01, 0xC0, 0x02};
    const uint8_t v2[] = {0x01, 0xDB, 0x02};
    const uint8_t v3[] = {0x01, 0x02};
    /* Several END (0xC0) and ESC (0xDB) bytes interleaved with plain bytes. */
    const uint8_t v4[] = {0xC0, 0xDB, 0x00, 0xC0, 0xC0, 0xDB, 0xFF, 0xDB, 0xC0};
    uint8_t enc[64];
    uint8_t dec[64];
    size_t enc_n, dec_n;

    /* v1 */
    enc_n = slip_encode(v1, sizeof(v1), enc, sizeof(enc));
    TEST_ASSERT_GREATER_THAN_size_t(0, enc_n);
    dec_n = slip_decode(enc, enc_n, dec, sizeof(dec));
    TEST_ASSERT_EQUAL_size_t(sizeof(v1), dec_n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(v1, dec, sizeof(v1));

    /* v2 */
    enc_n = slip_encode(v2, sizeof(v2), enc, sizeof(enc));
    TEST_ASSERT_GREATER_THAN_size_t(0, enc_n);
    dec_n = slip_decode(enc, enc_n, dec, sizeof(dec));
    TEST_ASSERT_EQUAL_size_t(sizeof(v2), dec_n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(v2, dec, sizeof(v2));

    /* v3 */
    enc_n = slip_encode(v3, sizeof(v3), enc, sizeof(enc));
    TEST_ASSERT_GREATER_THAN_size_t(0, enc_n);
    dec_n = slip_decode(enc, enc_n, dec, sizeof(dec));
    TEST_ASSERT_EQUAL_size_t(sizeof(v3), dec_n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(v3, dec, sizeof(v3));

    /* v4: heavy END/ESC stuffing. */
    enc_n = slip_encode(v4, sizeof(v4), enc, sizeof(enc));
    TEST_ASSERT_GREATER_THAN_size_t(0, enc_n);
    dec_n = slip_decode(enc, enc_n, dec, sizeof(dec));
    TEST_ASSERT_EQUAL_size_t(sizeof(v4), dec_n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(v4, dec, sizeof(v4));
}

/* Bullet 6: decode({0x01,0xC0,0x99},3) -> {0x01}, returns 1
 * (stops at first unescaped END, ignoring trailing bytes). */
static void test_slip_decode_stops_at_first_end(void) {
    const uint8_t in[]  = {0x01, 0xC0, 0x99};
    const uint8_t exp[] = {0x01};
    uint8_t out[16];

    size_t n = slip_decode(in, sizeof(in), out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(1, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

/* Bullet 7: malformed bad escape: decode({0x01,0xDB,0x99},3) -> 0
 * (ESC followed by a byte that is neither ESC_END nor ESC_ESC). */
static void test_slip_decode_bad_escape_returns_zero(void) {
    const uint8_t in[] = {0x01, 0xDB, 0x99};
    uint8_t out[16];

    TEST_ASSERT_EQUAL_size_t(0, slip_decode(in, sizeof(in), out, sizeof(out)));
}

/* Bullet 8: malformed no terminating END: decode({0x01,0x02},2) -> 0
 * (reaches end of input without an unescaped END). */
static void test_slip_decode_no_terminating_end_returns_zero(void) {
    const uint8_t in[] = {0x01, 0x02};
    uint8_t out[16];

    TEST_ASSERT_EQUAL_size_t(0, slip_decode(in, sizeof(in), out, sizeof(out)));
}

/* Bullet 9: out_size one byte short -> 0 (encode and decode), writes nothing. */
static void test_slip_output_one_byte_short_returns_zero(void) {
    /* encode({0x01,0xC0,0x02},3) needs 5 bytes; give it 4 -> must refuse. */
    const uint8_t enc_in[] = {0x01, 0xC0, 0x02};
    /* decode({0x01,0xDB,0xDC,0x02,0xC0},5) needs 3 bytes; give it 2 -> refuse. */
    const uint8_t dec_in[] = {0x01, 0xDB, 0xDC, 0x02, 0xC0};
    uint8_t out[16];
    const uint8_t sentinel = 0x5A;

    /* encode one byte short: nothing written. */
    out[0] = sentinel;
    TEST_ASSERT_EQUAL_size_t(0, slip_encode(enc_in, sizeof(enc_in), out, 4));
    TEST_ASSERT_EQUAL_UINT8(sentinel, out[0]);

    /* decode one byte short: nothing written. */
    out[0] = sentinel;
    TEST_ASSERT_EQUAL_size_t(0, slip_decode(dec_in, sizeof(dec_in), out, 2));
    TEST_ASSERT_EQUAL_UINT8(sentinel, out[0]);
}

/* Bullet 10 (part a): empty (len 0) -> 0 for both. */
static void test_slip_zero_length_returns_zero(void) {
    const uint8_t in[] = {0x01};
    uint8_t out[16];

    TEST_ASSERT_EQUAL_size_t(0, slip_encode(in, 0, out, sizeof(out)));
    TEST_ASSERT_EQUAL_size_t(0, slip_decode(in, 0, out, sizeof(out)));
}

/* Bullet 10 (part b): NULL data/slip/out -> 0, no crash, for both functions. */
static void test_slip_null_pointers_return_zero(void) {
    const uint8_t in[]  = {0x01, 0xC0, 0x02};
    const uint8_t enc[] = {0x01, 0xDB, 0xDC, 0x02, 0xC0};
    uint8_t out[16];

    /* NULL source. */
    TEST_ASSERT_EQUAL_size_t(0, slip_encode(NULL, sizeof(in), out, sizeof(out)));
    TEST_ASSERT_EQUAL_size_t(0, slip_decode(NULL, sizeof(enc), out, sizeof(out)));

    /* NULL output. */
    TEST_ASSERT_EQUAL_size_t(0, slip_encode(in, sizeof(in), NULL, sizeof(out)));
    TEST_ASSERT_EQUAL_size_t(0, slip_decode(enc, sizeof(enc), NULL, sizeof(out)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_slip_encode_escapes_end_byte);
    RUN_TEST(test_slip_encode_escapes_esc_byte);
    RUN_TEST(test_slip_encode_verbatim_with_trailing_end);
    RUN_TEST(test_slip_decode_unescapes_frame);
    RUN_TEST(test_slip_round_trip);
    RUN_TEST(test_slip_decode_stops_at_first_end);
    RUN_TEST(test_slip_decode_bad_escape_returns_zero);
    RUN_TEST(test_slip_decode_no_terminating_end_returns_zero);
    RUN_TEST(test_slip_output_one_byte_short_returns_zero);
    RUN_TEST(test_slip_zero_length_returns_zero);
    RUN_TEST(test_slip_null_pointers_return_zero);
    return UNITY_END();
}
