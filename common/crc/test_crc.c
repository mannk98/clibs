#include "unity.h"
#include "crc.h"

void setUp(void) {}
void tearDown(void) {}

/* Catalogue check string: ASCII "123456789", length 9 (no NUL). */
static const char CHECK[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

/* Bullet 1: ASCII "123456789" (len 9): crc8_maxim == 0xA1. */
static void test_crc_crc8_maxim_check_value(void) {
    TEST_ASSERT_EQUAL_HEX8(0xA1, crc8_maxim(CHECK, 9));
}

/* Bullet 2: ASCII "123456789": crc16_ccitt == 0x29B1. */
static void test_crc_crc16_ccitt_check_value(void) {
    TEST_ASSERT_EQUAL_HEX16(0x29B1, crc16_ccitt(CHECK, 9));
}

/* Bullet 3: ASCII "123456789": crc16_modbus == 0x4B37. */
static void test_crc_crc16_modbus_check_value(void) {
    TEST_ASSERT_EQUAL_HEX16(0x4B37, crc16_modbus(CHECK, 9));
}

/* Bullet 4: ASCII "123456789": crc32 == 0xCBF43926. */
static void test_crc_crc32_check_value(void) {
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, crc32(CHECK, 9));
}

/* Bullet 5: Empty (len 0): each CRC returns its zero-byte value. */
static void test_crc_empty_returns_init_values(void) {
    /* A non-NULL pointer with len 0 must still yield the zero-byte CRC. */
    TEST_ASSERT_EQUAL_HEX8(0x00, crc8_maxim(CHECK, 0));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_ccitt(CHECK, 0));
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, crc16_modbus(CHECK, 0));
    TEST_ASSERT_EQUAL_HEX32(0x00000000u, crc32(CHECK, 0));
}

/* Bullet 6: NULL data: crc8_maxim(NULL,5)==0x00, crc32(NULL,5)==0x00000000
 * (treated as zero bytes). */
static void test_crc_null_data_treated_as_zero_bytes(void) {
    TEST_ASSERT_EQUAL_HEX8(0x00, crc8_maxim(NULL, 5));
    TEST_ASSERT_EQUAL_HEX32(0x00000000u, crc32(NULL, 5));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_crc_crc8_maxim_check_value);
    RUN_TEST(test_crc_crc16_ccitt_check_value);
    RUN_TEST(test_crc_crc16_modbus_check_value);
    RUN_TEST(test_crc_crc32_check_value);
    RUN_TEST(test_crc_empty_returns_init_values);
    RUN_TEST(test_crc_null_data_treated_as_zero_bytes);
    return UNITY_END();
}
