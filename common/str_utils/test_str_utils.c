#include "unity.h"
#include "str_utils.h"

void setUp(void) {}
void tearDown(void) {}

static void test_str_len(void) {
    TEST_ASSERT_EQUAL_UINT16(0, str_len(""));
    TEST_ASSERT_EQUAL_UINT16(5, str_len("hello"));
    TEST_ASSERT_EQUAL_UINT16(0, str_len(NULL));
}

static void test_str_compare_n_equal_and_prefix(void) {
    TEST_ASSERT_EQUAL_INT(0, str_compare_n("abc", "abc", 3));
    TEST_ASSERT_EQUAL_INT(0, str_compare_n("abc", "abd", 2)); /* first 2 equal */
    TEST_ASSERT_TRUE(str_compare_n("abc", "abd", 3) < 0);     /* 'c' < 'd'      */
}

static void test_str_compare_n_no_uint8_truncation(void) {
    /* 'a'(97) vs 0x80(128): as unsigned char, 'a' < 0x80 -> negative. The old
     * uint8_t return would mangle this sign. */
    char hi[2] = { (char)0x80, 0 };
    char lo[2] = { 'a', 0 };
    TEST_ASSERT_TRUE(str_compare_n(lo, hi, 1) < 0);
}

static void test_str_cpy_n_truncates_and_terminates(void) {
    char dst[4] = { 'Z', 'Z', 'Z', 'Z' };
    str_cpy_n(dst, "hello", 4);
    TEST_ASSERT_EQUAL_STRING("hel", dst); /* 3 chars + NUL, never overruns */
}

static void test_str_cpy_n_fits(void) {
    char dst[8];
    str_cpy_n(dst, "hi", 8);
    TEST_ASSERT_EQUAL_STRING("hi", dst);
}

static void test_str_cpy_n_n_equals_one(void) {
    char dst[2] = { 'X', 'X' };
    str_cpy_n(dst, "hello", 1);
    TEST_ASSERT_EQUAL_INT('\0', dst[0]); /* no data bytes fit, only NUL */
    TEST_ASSERT_EQUAL_INT('X',  dst[1]); /* never written past n */
}

static void test_str_cpy_n_null_src(void) {
    char dst[4] = { 'Z', 'Z', 'Z', 'Z' };
    str_cpy_n(dst, NULL, 4);
    TEST_ASSERT_EQUAL_INT('\0', dst[0]); /* null src -> empty string */
}

static void test_str_reverse(void) {
    char s[] = "abcd";
    str_reverse(s);
    TEST_ASSERT_EQUAL_STRING("dcba", s);
    char one[] = "x";
    str_reverse(one);
    TEST_ASSERT_EQUAL_STRING("x", one);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_str_len);
    RUN_TEST(test_str_compare_n_equal_and_prefix);
    RUN_TEST(test_str_compare_n_no_uint8_truncation);
    RUN_TEST(test_str_cpy_n_truncates_and_terminates);
    RUN_TEST(test_str_cpy_n_fits);
    RUN_TEST(test_str_cpy_n_n_equals_one);
    RUN_TEST(test_str_cpy_n_null_src);
    RUN_TEST(test_str_reverse);
    return UNITY_END();
}
