/* Host Unity test for the pure TM1637 encode part (tm1637.c).
 * RED step: the header/impl do not exist yet — this test compiles against
 * the API contract from the story C15-2 and must fail to link until the Dev
 * provides tm1637.c (GREEN step). No SDK glue (tm1637_dev.*) is tested here;
 * that is xtensa COMPILE_GATE deferred. */
#include "unity.h"
#include "tm1637.h"   /* pure header — does not exist yet (RED) */

void setUp(void) {}
void tearDown(void) {}

/* Acceptance: tm1637_digit_segments: 0->0x3F, 8->0x7F, 9->0x6F, 10->0x00 (>9 blanks).
 * Asserts the full 0..9 table per the contract plus the out-of-range blank. */
static void test_tm1637_digit_segments_table_and_blank(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x3F, tm1637_digit_segments(0));
    TEST_ASSERT_EQUAL_HEX8(0x06, tm1637_digit_segments(1));
    TEST_ASSERT_EQUAL_HEX8(0x5B, tm1637_digit_segments(2));
    TEST_ASSERT_EQUAL_HEX8(0x4F, tm1637_digit_segments(3));
    TEST_ASSERT_EQUAL_HEX8(0x66, tm1637_digit_segments(4));
    TEST_ASSERT_EQUAL_HEX8(0x6D, tm1637_digit_segments(5));
    TEST_ASSERT_EQUAL_HEX8(0x7D, tm1637_digit_segments(6));
    TEST_ASSERT_EQUAL_HEX8(0x07, tm1637_digit_segments(7));
    TEST_ASSERT_EQUAL_HEX8(0x7F, tm1637_digit_segments(8));
    TEST_ASSERT_EQUAL_HEX8(0x6F, tm1637_digit_segments(9));
    TEST_ASSERT_EQUAL_HEX8(0x00, tm1637_digit_segments(10));  /* >9 -> blank */
}

/* Acceptance: tm1637_encode_number(1234,false,out) -> out == {0x06,0x5B,0x4F,0x66}.
 * thousands=1, hundreds=2, tens=3, ones=4 (all non-blank). */
static void test_tm1637_encode_number_1234_no_leading_zeros(void)
{
    uint8_t out[4] = {0xAA, 0xAA, 0xAA, 0xAA};  /* poison so we see real writes */
    tm1637_encode_number(1234, false, out);
    TEST_ASSERT_EQUAL_HEX8(0x06, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x5B, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x4F, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x66, out[3]);
}

/* Acceptance: tm1637_encode_number(42,false,out) -> out == {0x00,0x00,0x66,0x5B}
 * (leading zeros blanked: thousands+hundreds blank, tens=4, ones=2). */
static void test_tm1637_encode_number_42_leading_zeros_blanked(void)
{
    uint8_t out[4] = {0xAA, 0xAA, 0xAA, 0xAA};
    tm1637_encode_number(42, false, out);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x66, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x5B, out[3]);
}

/* Acceptance: tm1637_encode_number(42,true,out) -> out == {0x3F,0x3F,0x66,0x5B}
 * (leading zeros shown as '0' = 0x3F). */
static void test_tm1637_encode_number_42_leading_zeros_shown(void)
{
    uint8_t out[4] = {0xAA, 0xAA, 0xAA, 0xAA};
    tm1637_encode_number(42, true, out);
    TEST_ASSERT_EQUAL_HEX8(0x3F, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x3F, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x66, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x5B, out[3]);
}

/* Acceptance: tm1637_encode_number(0,false,out) -> out == {0x00,0x00,0x00,0x3F}
 * (ones digit is always shown even with leading_zeros=false). */
static void test_tm1637_encode_number_zero_ones_always_shown(void)
{
    uint8_t out[4] = {0xAA, 0xAA, 0xAA, 0xAA};
    tm1637_encode_number(0, false, out);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x3F, out[3]);
}

/* Acceptance: tm1637_encode_number(1234,false,NULL) -> no-op (no crash).
 * NULL out must be guarded; reaching the next line proves no crash. */
static void test_tm1637_encode_number_null_out_is_noop(void)
{
    tm1637_encode_number(1234, false, NULL);
    TEST_PASS();  /* survived the NULL call without crashing */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_tm1637_digit_segments_table_and_blank);
    RUN_TEST(test_tm1637_encode_number_1234_no_leading_zeros);
    RUN_TEST(test_tm1637_encode_number_42_leading_zeros_blanked);
    RUN_TEST(test_tm1637_encode_number_42_leading_zeros_shown);
    RUN_TEST(test_tm1637_encode_number_zero_ones_always_shown);
    RUN_TEST(test_tm1637_encode_number_null_out_is_noop);
    return UNITY_END();
}
