#include "unity.h"
#include "json_build.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_basic_object(void)
{
    char buf[64];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    json_build_str(&b, "state", "on");
    json_build_int(&b, "rssi", -60);
    int len = json_build_end(&b);
    TEST_ASSERT_EQUAL_STRING("{\"state\":\"on\",\"rssi\":-60}", buf);
    TEST_ASSERT_EQUAL_INT((int) strlen(buf), len);
}

static void test_raw_member(void)
{
    char buf[32];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    json_build_raw(&b, "ok", "true");
    int len = json_build_end(&b);
    TEST_ASSERT_EQUAL_STRING("{\"ok\":true}", buf);
    TEST_ASSERT_TRUE(len > 0);
}

static void test_empty_object(void)
{
    char buf[8];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    int len = json_build_end(&b);
    TEST_ASSERT_EQUAL_STRING("{}", buf);
    TEST_ASSERT_EQUAL_INT(2, len);
}

static void test_escaping(void)
{
    char buf[32];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    json_build_str(&b, "k", "a\"b\\c");
    (void) json_build_end(&b);
    TEST_ASSERT_EQUAL_STRING("{\"k\":\"a\\\"b\\\\c\"}", buf);
}

static void test_overflow(void)
{
    char buf[8];
    json_build b;
    json_build_init(&b, buf, sizeof buf);
    json_build_str(&b, "state", "on");   /* will not fit in 8 bytes */
    int len = json_build_end(&b);
    TEST_ASSERT_EQUAL_INT(-1, len);
    TEST_ASSERT_TRUE(b.err);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_basic_object);
    RUN_TEST(test_raw_member);
    RUN_TEST(test_empty_object);
    RUN_TEST(test_escaping);
    RUN_TEST(test_overflow);
    return UNITY_END();
}
