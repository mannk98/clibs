#include "unity.h"
#include "dht_parse.h"

void setUp(void) {}
void tearDown(void) {}

static void test_valid_frame(void)
{
    /* humidity 0x0292=658 (65.8%), temp 0x014A=330 (33.0C), checksum 0xDF */
    uint8_t f[5] = {0x02, 0x92, 0x01, 0x4A, 0xDF};
    int16_t t = 0, h = 0;
    TEST_ASSERT_TRUE(dht_parse(f, &t, &h));
    TEST_ASSERT_EQUAL_INT16(658, h);
    TEST_ASSERT_EQUAL_INT16(330, t);
}

static void test_negative_temp(void)
{
    /* temp byte 0x80 sets the sign bit: -(0x0014)= -20 (-2.0C); checksum 0x28 */
    uint8_t f[5] = {0x02, 0x92, 0x80, 0x14, 0x28};
    int16_t t = 0, h = 0;
    TEST_ASSERT_TRUE(dht_parse(f, &t, &h));
    TEST_ASSERT_EQUAL_INT16(-20, t);
    TEST_ASSERT_EQUAL_INT16(658, h);
}

static void test_bad_checksum(void)
{
    uint8_t f[5] = {0x02, 0x92, 0x01, 0x4A, 0x00};   /* wrong checksum */
    int16_t t = 0, h = 0;
    TEST_ASSERT_FALSE(dht_parse(f, &t, &h));
}

static void test_null(void)
{
    uint8_t f[5] = {0};
    int16_t t = 0, h = 0;
    TEST_ASSERT_FALSE(dht_parse(NULL, &t, &h));
    TEST_ASSERT_FALSE(dht_parse(f, NULL, &h));
    TEST_ASSERT_FALSE(dht_parse(f, &t, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_frame);
    RUN_TEST(test_negative_temp);
    RUN_TEST(test_bad_checksum);
    RUN_TEST(test_null);
    return UNITY_END();
}
