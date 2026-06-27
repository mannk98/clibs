#include "unity.h"
#include "base64.h"

void setUp(void) {}
void tearDown(void) {}

/* RFC 4648 sec. 10 test vectors (encode): "foobar" produces
 * the canonical "Zm9vYmFy"/"Zm8="/"Zg==" outputs covering 0/1/2 pad. */

/* Bullet 1: encode std-alphabet vectors with '=' padding, no NUL written. */
static void test_base64_encode_rfc4648_vectors(void) {
    char out[16];

    /* "foobar" (6 bytes) -> "Zm9vYmFy" (8 chars, no padding). */
    TEST_ASSERT_EQUAL_size_t(8, base64_encode("foobar", 6, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR_ARRAY("Zm9vYmFy", out, 8);

    /* "fo" (2 bytes) -> "Zm8=" (4 chars, one '=' pad). */
    TEST_ASSERT_EQUAL_size_t(4, base64_encode("fo", 2, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR_ARRAY("Zm8=", out, 4);

    /* "f" (1 byte) -> "Zg==" (4 chars, two '=' pad). */
    TEST_ASSERT_EQUAL_size_t(4, base64_encode("f", 1, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR_ARRAY("Zg==", out, 4);
}

/* Bullet 2: decode std-alphabet vectors handling 0/1/2 '=' padding. */
static void test_base64_decode_rfc4648_vectors(void) {
    char out[16];

    /* "Zm9vYmFy" (8 chars) -> "foobar" (6 bytes). */
    TEST_ASSERT_EQUAL_size_t(6, base64_decode("Zm9vYmFy", 8, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR_ARRAY("foobar", out, 6);

    /* "Zm8=" (one '=') -> "fo" (2 bytes). */
    TEST_ASSERT_EQUAL_size_t(2, base64_decode("Zm8=", 4, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR_ARRAY("fo", out, 2);

    /* "Zg==" (two '=') -> "f" (1 byte). */
    TEST_ASSERT_EQUAL_size_t(1, base64_decode("Zg==", 4, out, sizeof(out)));
    TEST_ASSERT_EQUAL_CHAR_ARRAY("f", out, 1);
}

/* Bullet 3: decode with b64_len not a multiple of 4 -> 0, writes nothing. */
static void test_base64_decode_rejects_bad_length(void) {
    char out[8] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};

    TEST_ASSERT_EQUAL_size_t(0, base64_decode("abc", 3, out, sizeof(out)));
    /* Nothing must have been written. */
    for (size_t i = 0; i < sizeof(out); i++) {
        TEST_ASSERT_EQUAL_CHAR('X', out[i]);
    }
}

/* Bullet 4: decode with invalid (non-alphabet) chars -> 0, writes nothing. */
static void test_base64_decode_rejects_invalid_chars(void) {
    char out[8] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};

    TEST_ASSERT_EQUAL_size_t(0, base64_decode("####", 4, out, sizeof(out)));
    for (size_t i = 0; i < sizeof(out); i++) {
        TEST_ASSERT_EQUAL_CHAR('X', out[i]);
    }
}

/* Bullet 5: encode with out_size < 4*ceil(len/3) -> 0, writes nothing.
 * "foobar" needs 8 chars; give 7. */
static void test_base64_encode_rejects_small_output(void) {
    char out[8] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};

    TEST_ASSERT_EQUAL_size_t(0, base64_encode("foobar", 6, out, 7));
    for (size_t i = 0; i < sizeof(out); i++) {
        TEST_ASSERT_EQUAL_CHAR('X', out[i]);
    }
}

/* Bullet 6: empty input (len 0 / b64_len 0) -> 0 for both encode and decode. */
static void test_base64_empty_input_returns_zero(void) {
    char out[8];

    TEST_ASSERT_EQUAL_size_t(0, base64_encode("foobar", 0, out, sizeof(out)));
    TEST_ASSERT_EQUAL_size_t(0, base64_decode("Zm9vYmFy", 0, out, sizeof(out)));
}

/* Bullet 7: NULL data / NULL out -> 0, no crash, for both functions. */
static void test_base64_null_pointers_return_zero(void) {
    char out[8];

    /* NULL source data. */
    TEST_ASSERT_EQUAL_size_t(0, base64_encode(NULL, 6, out, sizeof(out)));
    TEST_ASSERT_EQUAL_size_t(0, base64_decode(NULL, 8, out, sizeof(out)));

    /* NULL output buffer. */
    TEST_ASSERT_EQUAL_size_t(0, base64_encode("foobar", 6, NULL, 8));
    TEST_ASSERT_EQUAL_size_t(0, base64_decode("Zm9vYmFy", 8, NULL, 8));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_base64_encode_rfc4648_vectors);
    RUN_TEST(test_base64_decode_rfc4648_vectors);
    RUN_TEST(test_base64_decode_rejects_bad_length);
    RUN_TEST(test_base64_decode_rejects_invalid_chars);
    RUN_TEST(test_base64_encode_rejects_small_output);
    RUN_TEST(test_base64_empty_input_returns_zero);
    RUN_TEST(test_base64_null_pointers_return_zero);
    return UNITY_END();
}
