/* Host Unity test for the pure PCF8574 4-bit nibble-framing part of the
 * lcd1602 driver (lcd1602_encode_byte).
 *
 * RED step: lcd1602.h / lcd1602.c do not exist yet — this test compiles
 * against the API contract from story C15-3 and must FAIL to link until
 * the Dev provides lcd1602.c (GREEN step).
 *
 * Only the pure part is exercised here. The SDK glue (lcd1602_dev.{h,c},
 * which pulls in esp_err.h / i2c_bus.h / rom/ets_sys.h) is host-deferred
 * (xtensa COMPILE_GATE) and is NOT included or tested in this file. */
#include "unity.h"
#include "lcd1602.h"   /* pure header: only <stdint.h>/<stdbool.h>, no SDK */

void setUp(void)    {}
void tearDown(void) {}

/* PCF8574 wiring per contract: P0=RS, P1=RW=0, P2=EN, P3=backlight,
 * P4..P7=D4..D7. encode_byte clocks ONE HD44780 byte in 4-bit mode into
 * four PCF8574 bytes: {hi|base|EN, hi|base, lo|base|EN, lo|base} where
 *   hi   = value & 0xF0
 *   lo   = (value << 4) & 0xF0
 *   base = (rs?0x01:0) | (backlight?0x08:0)
 *   EN   = 0x04 */

/* Acceptance 1:
 * encode_byte(0x41,true,true,out) -> {0x4D,0x49,0x1D,0x19}
 * ('A', data write, backlight on). */
static void test_lcd1602_encode_byte_data_char_backlight_on(void)
{
    uint8_t out[4] = {0xAA, 0xBB, 0xCC, 0xDD};  /* poison to see real writes */
    lcd1602_encode_byte(0x41, true, true, out);
    TEST_ASSERT_EQUAL_HEX8(0x4D, out[0]);  /* hi nibble, EN high  */
    TEST_ASSERT_EQUAL_HEX8(0x49, out[1]);  /* hi nibble, EN low   */
    TEST_ASSERT_EQUAL_HEX8(0x1D, out[2]);  /* lo nibble, EN high  */
    TEST_ASSERT_EQUAL_HEX8(0x19, out[3]);  /* lo nibble, EN low   */
}

/* Acceptance 2:
 * encode_byte(0x01,false,true,out) -> {0x0C,0x08,0x1C,0x18}
 * (clear-display command, rs=0, backlight on). */
static void test_lcd1602_encode_byte_command_backlight_on(void)
{
    uint8_t out[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    lcd1602_encode_byte(0x01, false, true, out);
    TEST_ASSERT_EQUAL_HEX8(0x0C, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x08, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x1C, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x18, out[3]);
}

/* Acceptance 3:
 * encode_byte(0x41,false,false,out) -> {0x44,0x40,0x14,0x10}
 * ('A' as a command, backlight off — RS clear AND backlight clear). */
static void test_lcd1602_encode_byte_command_backlight_off(void)
{
    uint8_t out[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    lcd1602_encode_byte(0x41, false, false, out);
    TEST_ASSERT_EQUAL_HEX8(0x44, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x40, out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x14, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x10, out[3]);
}

/* Acceptance 4:
 * encode_byte(0x41,true,true,NULL) -> no-op, must not crash. */
static void test_lcd1602_encode_byte_null_out_is_noop(void)
{
    /* If this dereferences NULL the harness segfaults; reaching the PASS
     * proves the NULL guard. */
    lcd1602_encode_byte(0x41, true, true, NULL);
    TEST_PASS();
}

/* Acceptance 5:
 * out[0] and out[2] carry EN=0x04 set (latch-high nibbles);
 * out[1] and out[3] carry EN clear (latch-low nibbles). */
static void test_lcd1602_encode_byte_en_pulse_pattern(void)
{
    uint8_t out[4] = {0x00, 0x00, 0x00, 0x00};
    lcd1602_encode_byte(0x41, true, true, out);
    TEST_ASSERT_EQUAL_HEX8(0x04, out[0] & 0x04);  /* EN high on latch-high */
    TEST_ASSERT_EQUAL_HEX8(0x00, out[1] & 0x04);  /* EN clear              */
    TEST_ASSERT_EQUAL_HEX8(0x04, out[2] & 0x04);  /* EN high on latch-high */
    TEST_ASSERT_EQUAL_HEX8(0x00, out[3] & 0x04);  /* EN clear              */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lcd1602_encode_byte_data_char_backlight_on);
    RUN_TEST(test_lcd1602_encode_byte_command_backlight_on);
    RUN_TEST(test_lcd1602_encode_byte_command_backlight_off);
    RUN_TEST(test_lcd1602_encode_byte_null_out_is_noop);
    RUN_TEST(test_lcd1602_encode_byte_en_pulse_pattern);
    return UNITY_END();
}
