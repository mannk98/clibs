#include "unity.h"
#include "crc.h"

void setUp(void) {}
void tearDown(void) {}

/* CRC catalogue "check" input: ASCII "123456789". */
static const uint8_t CHK[9] = {'1','2','3','4','5','6','7','8','9'};

static void test_crc8_maxim_check(void)
{
    TEST_ASSERT_EQUAL_HEX8(0xA1, crc8_maxim(CHK, 9));
}

static void test_crc16_modbus_check(void)
{
    TEST_ASSERT_EQUAL_HEX16(0x4B37, crc16_modbus(CHK, 9));
}

static void test_empty(void)
{
    uint8_t d = 0;
    TEST_ASSERT_EQUAL_HEX8(0x00, crc8_maxim(&d, 0));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_modbus(&d, 0));
}

static void test_null(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x00, crc8_maxim(NULL, 5));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_modbus(NULL, 5));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc8_maxim_check);
    RUN_TEST(test_crc16_modbus_check);
    RUN_TEST(test_empty);
    RUN_TEST(test_null);
    return UNITY_END();
}
