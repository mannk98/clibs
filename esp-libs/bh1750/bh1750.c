#include "bh1750.h"

bool bh1750_parse(const uint8_t raw[2], uint32_t *milli_lux)
{
    if (raw == NULL || milli_lux == NULL) {
        return false;
    }
    uint32_t val = ((uint32_t)raw[0] << 8) | raw[1];
    *milli_lux = val * 2500u / 3u;
    return true;
}
