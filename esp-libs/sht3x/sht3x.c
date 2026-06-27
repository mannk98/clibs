#include "sht3x.h"

/* Sensirion CRC8: polynomial 0x31, init 0xFF, MSB-first, no final XOR.
 * Operates on non-negative uint8 only (no signed-shift UB). */
static uint8_t sht3x_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x31u)
                                : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

bool sht3x_parse(const uint8_t raw[6], int32_t *temp_mc, int32_t *humi_mrh)
{
    if (raw == NULL || temp_mc == NULL || humi_mrh == NULL) {
        return false;
    }

    if (sht3x_crc8(&raw[0], 2) != raw[2] ||
        sht3x_crc8(&raw[3], 2) != raw[5]) {
        return false;
    }

    uint32_t raw_t = ((uint32_t)raw[0] << 8) | raw[1];
    uint32_t raw_h = ((uint32_t)raw[3] << 8) | raw[4];

    *temp_mc  = -45000 + (int32_t)((int64_t)175000 * raw_t / 65535);
    *humi_mrh =          (int32_t)((int64_t)100000 * raw_h / 65535);
    return true;
}
