/* Unity host test for the `hex` lib (RED step of TDD).
 *
 * Tests ONLY the frozen API contract from the batch-2 design spec
 * (docs/superpowers/specs/2026-06-27-clibs-common-oop-batch2-design.md,
 * "Module specs -> hex"). The production header/impl do not exist yet, so
 * this is expected to FAIL to compile/link (missing hex.h / undefined
 * references to hex_encode|hex_decode). No production code is written here.
 *
 * Frozen contract under test:
 *   size_t hex_encode(const void *data, size_t len, char *out, size_t out_size);
 *   size_t hex_decode(const char *hex, size_t hex_len, void *out, size_t out_size);
 * Returns: chars/bytes written; 0 = empty-or-error; no NUL is written.
 */
#include "unity.h"
#include "hex.h"

void setUp(void)    {}
void tearDown(void) {}

/* Bullet 1: hex_encode({0xDE,0xAD,0xBE,0xEF},4) with out_size>=8 ->
 *           "deadbeef" (lowercase, no NUL), returns 8. */
static void test_hex_encode_basic_lowercase(void) {
    const uint8_t in[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    char out[16];
    /* Pre-fill so we can verify no NUL is written by the encoder itself. */
    for (size_t i = 0; i < sizeof(out); i++) out[i] = 'X';
    size_t n = hex_encode(in, 4, out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(8, n);
    /* Exactly 8 chars, lowercase, no NUL within those 8. */
    TEST_ASSERT_EQUAL_CHAR_ARRAY("deadbeef", out, 8);
    /* Byte 8 must be untouched (no NUL termination written). */
    TEST_ASSERT_EQUAL_CHAR('X', out[8]);
}

/* Bullet 2: same input with out_size 7 -> returns 0, writes nothing. */
static void test_hex_encode_out_too_small(void) {
    const uint8_t in[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    char out[16];
    for (size_t i = 0; i < sizeof(out); i++) out[i] = 'X';
    size_t n = hex_encode(in, 4, out, 7);
    TEST_ASSERT_EQUAL_size_t(0, n);
    /* Nothing written: buffer remains entirely the sentinel. */
    for (size_t i = 0; i < sizeof(out); i++) {
        TEST_ASSERT_EQUAL_CHAR('X', out[i]);
    }
}

/* Bullet 3: hex_decode("deadbeef",8) -> {0xDE,0xAD,0xBE,0xEF}, returns 4. */
static void test_hex_decode_basic(void) {
    uint8_t out[8];
    for (size_t i = 0; i < sizeof(out); i++) out[i] = 0x55;
    size_t n = hex_decode("deadbeef", 8, out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(4, n);
    const uint8_t expect[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, out, 4);
}

/* Bullet 4: hex_decode("DEADBEEF",8) -> same bytes (upper+lower accepted),
 *           returns 4. */
static void test_hex_decode_uppercase(void) {
    uint8_t out[8];
    for (size_t i = 0; i < sizeof(out); i++) out[i] = 0x55;
    size_t n = hex_decode("DEADBEEF", 8, out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(4, n);
    const uint8_t expect[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect, out, 4);
}

/* Bullet 5: hex_decode("abc",3) -> 0 (odd length), writes nothing. */
static void test_hex_decode_odd_length(void) {
    uint8_t out[8];
    for (size_t i = 0; i < sizeof(out); i++) out[i] = 0x55;
    size_t n = hex_decode("abc", 3, out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(0, n);
    /* Nothing written. */
    for (size_t i = 0; i < sizeof(out); i++) {
        TEST_ASSERT_EQUAL_HEX8(0x55, out[i]);
    }
}

/* Bullet 6: hex_decode("zz",2) -> 0 (invalid char), writes nothing. */
static void test_hex_decode_invalid_char(void) {
    uint8_t out[8];
    for (size_t i = 0; i < sizeof(out); i++) out[i] = 0x55;
    size_t n = hex_decode("zz", 2, out, sizeof(out));
    TEST_ASSERT_EQUAL_size_t(0, n);
    for (size_t i = 0; i < sizeof(out); i++) {
        TEST_ASSERT_EQUAL_HEX8(0x55, out[i]);
    }
}

/* Bullet 7: hex_decode of 8 chars with out_size 1 -> 0 (out too small). */
static void test_hex_decode_out_too_small(void) {
    uint8_t out[8];
    for (size_t i = 0; i < sizeof(out); i++) out[i] = 0x55;
    size_t n = hex_decode("deadbeef", 8, out, 1);
    TEST_ASSERT_EQUAL_size_t(0, n);
    /* Out too small -> writes nothing. */
    for (size_t i = 0; i < sizeof(out); i++) {
        TEST_ASSERT_EQUAL_HEX8(0x55, out[i]);
    }
}

/* Bullet 8: empty (len 0 / hex_len 0) -> 0 for both. */
static void test_hex_empty_returns_zero(void) {
    const uint8_t in[1] = {0x00};
    char eout[16];
    uint8_t dout[8];
    TEST_ASSERT_EQUAL_size_t(0, hex_encode(in, 0, eout, sizeof(eout)));
    TEST_ASSERT_EQUAL_size_t(0, hex_decode("", 0, dout, sizeof(dout)));
}

/* Bullet 9: NULL data/out -> 0, no crash (both directions, every pointer arg). */
static void test_hex_null_guards(void) {
    const uint8_t in[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    char eout[16];
    uint8_t dout[8];
    /* encode: NULL data, NULL out. */
    TEST_ASSERT_EQUAL_size_t(0, hex_encode(NULL, 4, eout, sizeof(eout)));
    TEST_ASSERT_EQUAL_size_t(0, hex_encode(in, 4, NULL, sizeof(eout)));
    /* decode: NULL hex, NULL out. */
    TEST_ASSERT_EQUAL_size_t(0, hex_decode(NULL, 8, dout, sizeof(dout)));
    TEST_ASSERT_EQUAL_size_t(0, hex_decode("deadbeef", 8, NULL, sizeof(dout)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hex_encode_basic_lowercase);   /* bullet 1 */
    RUN_TEST(test_hex_encode_out_too_small);      /* bullet 2 */
    RUN_TEST(test_hex_decode_basic);              /* bullet 3 */
    RUN_TEST(test_hex_decode_uppercase);          /* bullet 4 */
    RUN_TEST(test_hex_decode_odd_length);         /* bullet 5 */
    RUN_TEST(test_hex_decode_invalid_char);       /* bullet 6 */
    RUN_TEST(test_hex_decode_out_too_small);      /* bullet 7 */
    RUN_TEST(test_hex_empty_returns_zero);        /* bullet 8 */
    RUN_TEST(test_hex_null_guards);               /* bullet 9 */
    return UNITY_END();
}
