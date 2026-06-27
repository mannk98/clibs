#include "ads1115.h"

/* Full-scale range in microvolts, indexed by ads1115_pga. */
static const int64_t ADS1115_FSR_UV[] = {
    6144000,  /* ADS1115_PGA_6V144 */
    4096000,  /* ADS1115_PGA_4V096 */
    2048000,  /* ADS1115_PGA_2V048 */
    1024000,  /* ADS1115_PGA_1V024 */
    512000,   /* ADS1115_PGA_0V512 */
    256000    /* ADS1115_PGA_0V256 */
};

#define ADS1115_PGA_COUNT \
    ((ads1115_pga)(sizeof(ADS1115_FSR_UV) / sizeof(ADS1115_FSR_UV[0])))

int32_t ads1115_to_microvolts(int16_t raw, ads1115_pga pga)
{
    /* Clamp an out-of-range PGA to the device default (2.048 V) rather than
     * indexing the FSR table out of bounds. */
    if (pga >= ADS1115_PGA_COUNT) {
        pga = ADS1115_PGA_2V048;
    }

    /* Widen the raw count to int64 BEFORE the multiply: -32768 * 6144000 does
     * not fit in 32 bits. The final value fits int32 (max |raw*FSR/32768| is the
     * FSR itself). Integer division truncates toward zero. */
    return (int32_t)((int64_t)raw * ADS1115_FSR_UV[pga] / 32768);
}

uint16_t ads1115_config(uint8_t channel, ads1115_pga pga)
{
    return (uint16_t)(0x8000u
                      | ((uint16_t)(0x4u | (channel & 0x3u)) << 12)
                      | ((uint16_t)pga << 9)
                      | 0x0100u   /* MODE = single-shot */
                      | 0x0080u   /* DR = 128 SPS */
                      | 0x0003u); /* COMP_QUE = disabled */
}
