#include "unity.h"
#include "hcsr04_dist.h"

void setUp(void) {}
void tearDown(void) {}

static void test_dist(void)
{
    TEST_ASSERT_EQUAL_UINT32(0,    hcsr04_us_to_mm(0));
    TEST_ASSERT_EQUAL_UINT32(994,  hcsr04_us_to_mm(5800));
    TEST_ASSERT_EQUAL_UINT32(9,    hcsr04_us_to_mm(58));
    TEST_ASSERT_EQUAL_UINT32(6517, hcsr04_us_to_mm(38000));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_dist);
    return UNITY_END();
}
