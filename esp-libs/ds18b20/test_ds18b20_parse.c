#include "unity.h"
#include "ds18b20_parse.h"
#include "crc.h"

void setUp(void) {}
void tearDown(void) {}

static void test_valid_85c(void)
{
    /* DS18B20 power-on default scratchpad; raw 0x0550 = 1360 -> 85000 mC */
    uint8_t s[9] = {0x50, 0x05, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0};
    s[8] = crc8_maxim(s, 8);
    int32_t t = 0;
    TEST_ASSERT_TRUE(ds18b20_parse_temp(s, &t));
    TEST_ASSERT_EQUAL_INT32(85000, t);
}

static void test_negative_10c(void)
{
    /* raw 0xFF60 = -160 -> -10000 mC */
    uint8_t s[9] = {0x60, 0xFF, 0, 0, 0, 0, 0, 0, 0};
    s[8] = crc8_maxim(s, 8);
    int32_t t = 0;
    TEST_ASSERT_TRUE(ds18b20_parse_temp(s, &t));
    TEST_ASSERT_EQUAL_INT32(-10000, t);
}

static void test_bad_crc(void)
{
    uint8_t s[9] = {0x50, 0x05, 0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0};
    s[8] = crc8_maxim(s, 8);
    s[0] ^= 0xFF;   /* corrupt data, CRC now mismatches */
    int32_t t = 0;
    TEST_ASSERT_FALSE(ds18b20_parse_temp(s, &t));
}

static void test_null(void)
{
    uint8_t s[9] = {0};
    int32_t t = 0;
    TEST_ASSERT_FALSE(ds18b20_parse_temp(NULL, &t));
    TEST_ASSERT_FALSE(ds18b20_parse_temp(s, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_85c);
    RUN_TEST(test_negative_10c);
    RUN_TEST(test_bad_crc);
    RUN_TEST(test_null);
    return UNITY_END();
}
