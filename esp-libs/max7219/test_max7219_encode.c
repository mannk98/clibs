/* Host Unity test for the pure MAX7219 encode part (max7219_encode).
 * RED step: the header/impl do not exist yet — this test compiles against
 * the API contract from the spec and must fail to link until the Dev
 * provides max7219_encode.c (GREEN step). No SDK glue is tested here. */
#include "unity.h"
#include "max7219_encode.h"

void setUp(void) {}
void tearDown(void) {}

/* Acceptance: frame(0x01,0x7E,out) -> out == {0x01, 0x7E} (reg, data; MSB-first). */
static void test_max7219_frame_packs_reg_then_data(void)
{
    uint8_t out[2] = {0xAA, 0xBB};   /* poison so we see real writes */
    max7219_frame(0x01, 0x7E, out);
    TEST_ASSERT_EQUAL_HEX8(0x01, out[0]);   /* register address first */
    TEST_ASSERT_EQUAL_HEX8(0x7E, out[1]);   /* data second */
}

/* Acceptance: digit_segments: 0->0x7E,1->0x30,2->0x6D,3->0x79,8->0x7F,9->0x7B.
 * (The full no-decode table from the contract is asserted, dp=false.) */
static void test_max7219_digit_segments_table(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x7E, max7219_digit_segments(0, false));
    TEST_ASSERT_EQUAL_HEX8(0x30, max7219_digit_segments(1, false));
    TEST_ASSERT_EQUAL_HEX8(0x6D, max7219_digit_segments(2, false));
    TEST_ASSERT_EQUAL_HEX8(0x79, max7219_digit_segments(3, false));
    TEST_ASSERT_EQUAL_HEX8(0x33, max7219_digit_segments(4, false));
    TEST_ASSERT_EQUAL_HEX8(0x5B, max7219_digit_segments(5, false));
    TEST_ASSERT_EQUAL_HEX8(0x5F, max7219_digit_segments(6, false));
    TEST_ASSERT_EQUAL_HEX8(0x70, max7219_digit_segments(7, false));
    TEST_ASSERT_EQUAL_HEX8(0x7F, max7219_digit_segments(8, false));
    TEST_ASSERT_EQUAL_HEX8(0x7B, max7219_digit_segments(9, false));
}

/* Acceptance: digit_segments(8,true) -> 0xFF (DP = bit7 ORed onto 0x7F). */
static void test_max7219_digit_segments_dp_sets_bit7(void)
{
    TEST_ASSERT_EQUAL_HEX8(0xFF, max7219_digit_segments(8, true));
}

/* Acceptance: digit_segments(0x0F,false) -> 0x00 (value out of 0..9 blanks). */
static void test_max7219_digit_segments_out_of_range_blank(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x00, max7219_digit_segments(0x0F, false));
}

/* Acceptance: digit_segments(0x0F,true) -> 0x80 (blank + DP bit7 only). */
static void test_max7219_digit_segments_out_of_range_blank_with_dp(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x80, max7219_digit_segments(0x0F, true));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max7219_frame_packs_reg_then_data);
    RUN_TEST(test_max7219_digit_segments_table);
    RUN_TEST(test_max7219_digit_segments_dp_sets_bit7);
    RUN_TEST(test_max7219_digit_segments_out_of_range_blank);
    RUN_TEST(test_max7219_digit_segments_out_of_range_blank_with_dp);
    return UNITY_END();
}
