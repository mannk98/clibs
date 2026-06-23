#include "hcsr04_dist.h"

uint32_t hcsr04_us_to_mm(uint32_t echo_us)
{
    return echo_us * 343u / 2000u;
}
