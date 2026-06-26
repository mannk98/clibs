#include "max6675_conv.h"

float max6675_raw_to_celsius(uint16_t raw) {
    if (raw & 0x4)            /* bit 2 set => no thermocouple attached */
        return MAX6675_OPEN;
    raw >>= 3;
    return raw * 0.25f;
}

float max6675_celsius_to_fahrenheit(float c) {
    return c * 9.0f / 5.0f + 32.0f;
}
