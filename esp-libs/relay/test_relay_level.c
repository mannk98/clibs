#include "unity.h"
#include "relay_level.h"

void setUp(void) {}
void tearDown(void) {}

static void test_active_high(void)
{
    TEST_ASSERT_EQUAL_INT(1, relay_level(true, false));
    TEST_ASSERT_EQUAL_INT(0, relay_level(false, false));
}

static void test_active_low(void)
{
    TEST_ASSERT_EQUAL_INT(0, relay_level(true, true));
    TEST_ASSERT_EQUAL_INT(1, relay_level(false, true));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_active_high);
    RUN_TEST(test_active_low);
    return UNITY_END();
}
