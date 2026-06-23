#include "unity.h"
#include "servo_duty.h"

void setUp(void) {}
void tearDown(void) {}

static void test_servo_duty(void)
{
    TEST_ASSERT_EQUAL_UINT32(1000, servo_angle_to_duty(0));
    TEST_ASSERT_EQUAL_UINT32(1500, servo_angle_to_duty(90));
    TEST_ASSERT_EQUAL_UINT32(2000, servo_angle_to_duty(180));
    TEST_ASSERT_EQUAL_UINT32(2000, servo_angle_to_duty(200));   /* clamp >180 */
    TEST_ASSERT_EQUAL_UINT32(1250, servo_angle_to_duty(45));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_servo_duty);
    return UNITY_END();
}
