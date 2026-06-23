#include "unity.h"
#include "throttle.h"

void setUp(void) {}
void tearDown(void) {}

static void test_basic(void)
{
    throttle t;
    throttle_init(&t, 1000);
    TEST_ASSERT_TRUE(throttle_allow(&t, 0));        /* first call -> true */
    TEST_ASSERT_FALSE(throttle_allow(&t, 500));     /* < interval */
    TEST_ASSERT_TRUE(throttle_allow(&t, 1000));     /* == interval */
    TEST_ASSERT_FALSE(throttle_allow(&t, 1500));
    TEST_ASSERT_TRUE(throttle_allow(&t, 2000));
}

static void test_wraparound(void)
{
    throttle t;
    throttle_init(&t, 10);
    TEST_ASSERT_TRUE(throttle_allow(&t, 0xFFFFFFF0u));   /* prime near wrap */
    TEST_ASSERT_TRUE(throttle_allow(&t, 0x00000005u));   /* elapsed 21 across the wrap >= 10 */
}

static void test_null(void)
{
    TEST_ASSERT_FALSE(throttle_allow(NULL, 100));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_basic);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_null);
    return UNITY_END();
}
