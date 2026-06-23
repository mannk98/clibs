#include "unity.h"
#include "device_id.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_format_with_prefix(void)
{
    uint8_t mac[6] = {0, 1, 2, 3, 4, 5};
    char buf[16];
    int len = device_id_format(mac, buf, sizeof buf, "esp-");
    TEST_ASSERT_EQUAL_INT(10, len);
    TEST_ASSERT_EQUAL_STRING("esp-030405", buf);   /* last 3 bytes: 03 04 05 */
}

static void test_format_no_prefix(void)
{
    uint8_t mac[6] = {0xaa, 0xbb, 0xcc, 0x3c, 0x71, 0xbf};
    char buf[16];
    int len = device_id_format(mac, buf, sizeof buf, NULL);
    TEST_ASSERT_EQUAL_INT(6, len);
    TEST_ASSERT_EQUAL_STRING("3c71bf", buf);
}

static void test_format_truncation(void)
{
    uint8_t mac[6] = {0, 1, 2, 3, 4, 5};
    char buf[4];
    TEST_ASSERT_EQUAL_INT(-1, device_id_format(mac, buf, sizeof buf, "esp-"));
    TEST_ASSERT_EQUAL_STRING("", buf);
}

static void test_format_null(void)
{
    char buf[16];
    uint8_t mac[6] = {0};
    TEST_ASSERT_EQUAL_INT(-1, device_id_format(NULL, buf, sizeof buf, "esp-"));
    TEST_ASSERT_EQUAL_INT(-1, device_id_format(mac, NULL, 16, "esp-"));
    TEST_ASSERT_EQUAL_INT(-1, device_id_format(mac, buf, 0, "esp-"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_format_with_prefix);
    RUN_TEST(test_format_no_prefix);
    RUN_TEST(test_format_truncation);
    RUN_TEST(test_format_null);
    return UNITY_END();
}
