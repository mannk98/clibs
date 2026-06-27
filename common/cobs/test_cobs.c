#include "unity.h"

/* Test only the frozen API contract — cobs.c does not exist yet (RED step).
 * Contract (from batch-3 design spec, CLIBS_COBS_H):
 *   size_t cobs_encode(const void *data, size_t len, uint8_t *out, size_t out_size);
 *   size_t cobs_decode(const void *cobs, size_t len, uint8_t *out, size_t out_size);
 * Both return the produced length, or 0 on NULL / len 0 / out too small / malformed.
 * Output of encode is zero-free; no trailing 0x00 delimiter (caller owns framing). */
#include "cobs.h"

void setUp(void) {}
void tearDown(void) {}

/* Bullet 1: encode({0x11,0x22,0x00,0x33},4) -> {0x03,0x11,0x22,0x02,0x33}, returns 5. */
static void test_cobs_encode_vector_with_zero(void) {
    const uint8_t in[]  = {0x11, 0x22, 0x00, 0x33};
    const uint8_t exp[] = {0x03, 0x11, 0x22, 0x02, 0x33};
    uint8_t out[16];

    size_t n = cobs_encode(in, sizeof(in), out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

/* Bullet 2: encode({0x11,0x22,0x33,0x44},4) -> {0x05,0x11,0x22,0x33,0x44}, returns 5. */
static void test_cobs_encode_vector_no_zero(void) {
    const uint8_t in[]  = {0x11, 0x22, 0x33, 0x44};
    const uint8_t exp[] = {0x05, 0x11, 0x22, 0x33, 0x44};
    uint8_t out[16];

    size_t n = cobs_encode(in, sizeof(in), out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(5, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

/* Bullet 3: encode({0x00},1) -> {0x01,0x01}, returns 2. */
static void test_cobs_encode_single_zero(void) {
    const uint8_t in[]  = {0x00};
    const uint8_t exp[] = {0x01, 0x01};
    uint8_t out[16];

    size_t n = cobs_encode(in, sizeof(in), out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(2, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

/* Bullet 4: every successful encode output contains NO 0x00 byte (zero-free).
 * Check across the canonical vectors AND the 300-byte all-0xAA buffer (which
 * forces a >254 run, exercising block boundaries). */
static void test_cobs_encode_output_is_zero_free(void) {
    /* Inputs that themselves contain zeros, to prove the codec really stuffs them. */
    const uint8_t v1[] = {0x11, 0x22, 0x00, 0x33};
    const uint8_t v2[] = {0x00, 0x00, 0x00};
    const uint8_t v3[] = {0x00};
    uint8_t big[300];
    uint8_t out[400];
    size_t n;

    n = cobs_encode(v1, sizeof(v1), out, sizeof(out));
    TEST_ASSERT_GREATER_THAN_size_t(0, n);
    for (size_t i = 0; i < n; i++) {
        TEST_ASSERT_NOT_EQUAL_UINT8(0x00, out[i]);
    }

    n = cobs_encode(v2, sizeof(v2), out, sizeof(out));
    TEST_ASSERT_GREATER_THAN_size_t(0, n);
    for (size_t i = 0; i < n; i++) {
        TEST_ASSERT_NOT_EQUAL_UINT8(0x00, out[i]);
    }

    n = cobs_encode(v3, sizeof(v3), out, sizeof(out));
    TEST_ASSERT_GREATER_THAN_size_t(0, n);
    for (size_t i = 0; i < n; i++) {
        TEST_ASSERT_NOT_EQUAL_UINT8(0x00, out[i]);
    }

    for (size_t i = 0; i < sizeof(big); i++) {
        big[i] = 0xAA;
    }
    n = cobs_encode(big, sizeof(big), out, sizeof(out));
    TEST_ASSERT_GREATER_THAN_size_t(0, n);
    for (size_t i = 0; i < n; i++) {
        TEST_ASSERT_NOT_EQUAL_UINT8(0x00, out[i]);
    }
}

/* Bullet 5: decode({0x03,0x11,0x22,0x02,0x33},5) -> {0x11,0x22,0x00,0x33}, returns 4. */
static void test_cobs_decode_vector(void) {
    const uint8_t in[]  = {0x03, 0x11, 0x22, 0x02, 0x33};
    const uint8_t exp[] = {0x11, 0x22, 0x00, 0x33};
    uint8_t out[16];

    size_t n = cobs_decode(in, sizeof(in), out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(4, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, sizeof(exp));
}

/* Bullet 6: round-trip decode(encode(x)) == x for the canonical vectors AND a
 * 300-byte all-0xAA buffer (254-block boundary; worst-case len + len/254 + 1). */
static void test_cobs_round_trip(void) {
    const uint8_t v1[] = {0x11, 0x22, 0x00, 0x33};
    const uint8_t v2[] = {0x11, 0x22, 0x33, 0x44};
    const uint8_t v3[] = {0x00};
    uint8_t big[300];
    uint8_t enc[400];
    uint8_t dec[400];
    size_t enc_n, dec_n;

    /* v1 */
    enc_n = cobs_encode(v1, sizeof(v1), enc, sizeof(enc));
    TEST_ASSERT_GREATER_THAN_size_t(0, enc_n);
    dec_n = cobs_decode(enc, enc_n, dec, sizeof(dec));
    TEST_ASSERT_EQUAL_size_t(sizeof(v1), dec_n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(v1, dec, sizeof(v1));

    /* v2 */
    enc_n = cobs_encode(v2, sizeof(v2), enc, sizeof(enc));
    TEST_ASSERT_GREATER_THAN_size_t(0, enc_n);
    dec_n = cobs_decode(enc, enc_n, dec, sizeof(dec));
    TEST_ASSERT_EQUAL_size_t(sizeof(v2), dec_n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(v2, dec, sizeof(v2));

    /* v3 */
    enc_n = cobs_encode(v3, sizeof(v3), enc, sizeof(enc));
    TEST_ASSERT_GREATER_THAN_size_t(0, enc_n);
    dec_n = cobs_decode(enc, enc_n, dec, sizeof(dec));
    TEST_ASSERT_EQUAL_size_t(sizeof(v3), dec_n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(v3, dec, sizeof(v3));

    /* 300-byte all-0xAA: crosses the 254-byte block boundary. */
    for (size_t i = 0; i < sizeof(big); i++) {
        big[i] = 0xAA;
    }
    enc_n = cobs_encode(big, sizeof(big), enc, sizeof(enc));
    TEST_ASSERT_GREATER_THAN_size_t(0, enc_n);
    /* Worst-case encoded size = len + len/254 + 1 = 300 + 1 + 1 = 302. */
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(big) + sizeof(big) / 254 + 1, enc_n);
    dec_n = cobs_decode(enc, enc_n, dec, sizeof(dec));
    TEST_ASSERT_EQUAL_size_t(sizeof(big), dec_n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(big, dec, sizeof(big));
}

/* Bullet 7: encode/decode with out_size one byte short -> 0. */
static void test_cobs_output_one_byte_short_returns_zero(void) {
    const uint8_t in[] = {0x11, 0x22, 0x00, 0x33};   /* encodes to 5 bytes. */
    const uint8_t enc[] = {0x03, 0x11, 0x22, 0x02, 0x33}; /* decodes to 4 bytes. */
    uint8_t out[8];

    /* encode needs 5 bytes; give it 4 -> must refuse with 0. */
    TEST_ASSERT_EQUAL_size_t(0, cobs_encode(in, sizeof(in), out, 4));

    /* decode needs 4 bytes; give it 3 -> must refuse with 0. */
    TEST_ASSERT_EQUAL_size_t(0, cobs_decode(enc, sizeof(enc), out, 3));
}

/* Bullet 8: malformed: decode({0x05,0x11},2) -> 0 (code byte overruns input). */
static void test_cobs_decode_malformed_overrun_returns_zero(void) {
    const uint8_t in[] = {0x05, 0x11};   /* code 0x05 claims 4 more bytes; only 1 present. */
    uint8_t out[16];

    TEST_ASSERT_EQUAL_size_t(0, cobs_decode(in, sizeof(in), out, sizeof(out)));
}

/* Bullet 9 (part a): edge len 0 -> 0 for both. */
static void test_cobs_zero_length_returns_zero(void) {
    const uint8_t in[] = {0x11};
    uint8_t out[16];

    TEST_ASSERT_EQUAL_size_t(0, cobs_encode(in, 0, out, sizeof(out)));
    TEST_ASSERT_EQUAL_size_t(0, cobs_decode(in, 0, out, sizeof(out)));
}

/* Bullet 9 (part b): any NULL pointer -> 0, no crash, for both functions. */
static void test_cobs_null_pointers_return_zero(void) {
    const uint8_t in[] = {0x11, 0x22, 0x00, 0x33};
    const uint8_t enc[] = {0x03, 0x11, 0x22, 0x02, 0x33};
    uint8_t out[16];

    /* NULL source. */
    TEST_ASSERT_EQUAL_size_t(0, cobs_encode(NULL, sizeof(in), out, sizeof(out)));
    TEST_ASSERT_EQUAL_size_t(0, cobs_decode(NULL, sizeof(enc), out, sizeof(out)));

    /* NULL output. */
    TEST_ASSERT_EQUAL_size_t(0, cobs_encode(in, sizeof(in), NULL, sizeof(out)));
    TEST_ASSERT_EQUAL_size_t(0, cobs_decode(enc, sizeof(enc), NULL, sizeof(out)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cobs_encode_vector_with_zero);
    RUN_TEST(test_cobs_encode_vector_no_zero);
    RUN_TEST(test_cobs_encode_single_zero);
    RUN_TEST(test_cobs_encode_output_is_zero_free);
    RUN_TEST(test_cobs_decode_vector);
    RUN_TEST(test_cobs_round_trip);
    RUN_TEST(test_cobs_output_one_byte_short_returns_zero);
    RUN_TEST(test_cobs_decode_malformed_overrun_returns_zero);
    RUN_TEST(test_cobs_zero_length_returns_zero);
    RUN_TEST(test_cobs_null_pointers_return_zero);
    return UNITY_END();
}
