#include "dimmer_duty.h"

uint32_t dimmer_duty(uint8_t percent, uint32_t max_duty)
{
    if (percent >= 100) {
        return max_duty;
    }
    return (uint32_t) (((uint64_t) max_duty * percent + 50) / 100);   /* rounded */
}
