#include "unity.h"
#include "dimmer_duty.h"

void setUp(void) {}
void tearDown(void) {}

static void test_zero(void)  { TEST_ASSERT_EQUAL_UINT32(0,    dimmer_duty(0,   1000)); }
static void test_half(void)  { TEST_ASSERT_EQUAL_UINT32(500,  dimmer_duty(50,  1000)); }
static void test_full(void)  { TEST_ASSERT_EQUAL_UINT32(1000, dimmer_duty(100, 1000)); }
static void test_clamp(void) { TEST_ASSERT_EQUAL_UINT32(1000, dimmer_duty(255, 1000)); }
static void test_round(void) { TEST_ASSERT_EQUAL_UINT32(33,   dimmer_duty(33,  100));  }

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_zero);
    RUN_TEST(test_half);
    RUN_TEST(test_full);
    RUN_TEST(test_clamp);
    RUN_TEST(test_round);
    return UNITY_END();
}
