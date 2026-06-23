#include "ds18b20_parse.h"

#include "crc.h"   /* L1 crc8_maxim */

bool ds18b20_parse_temp(const uint8_t scratch[9], int32_t *temp_mc)
{
    if (scratch == NULL || temp_mc == NULL) {
        return false;
    }
    if (crc8_maxim(scratch, 8) != scratch[8]) {
        return false;
    }
    int16_t raw = (int16_t) ((uint16_t) scratch[1] << 8 | scratch[0]);
    *temp_mc = (int32_t) raw * 625 / 10;   /* LSB = 0.0625 C = 62.5 mC */
    return true;
}
