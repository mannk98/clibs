#include "servo_duty.h"

uint32_t servo_angle_to_duty(uint8_t angle)
{
    if (angle > 180) {
        angle = 180;
    }
    return 1000u + (uint32_t) angle * 1000u / 180u;
}
