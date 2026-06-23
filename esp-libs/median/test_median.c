#include "unity.h"
#include "median.h"

void setUp(void) {}
void tearDown(void) {}

static void test_median3(void)
{
    TEST_ASSERT_EQUAL_INT32(2,  median3(1, 2, 3));
    TEST_ASSERT_EQUAL_INT32(2,  median3(3, 1, 2));
    TEST_ASSERT_EQUAL_INT32(5,  median3(5, 5, 1));
    TEST_ASSERT_EQUAL_INT32(-2, median3(-3, -1, -2));
    TEST_ASSERT_EQUAL_INT32(7,  median3(7, 7, 7));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_median3);
    return UNITY_END();
}
