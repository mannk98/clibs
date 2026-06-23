#include "dht_parse.h"
#include <stddef.h>

bool dht_parse(const uint8_t frame[5], int16_t *temp_dc, int16_t *humi_dpct)
{
    if (frame == NULL || temp_dc == NULL || humi_dpct == NULL) {
        return false;
    }
    uint8_t sum = (uint8_t) (frame[0] + frame[1] + frame[2] + frame[3]);
    if (sum != frame[4]) {
        return false;
    }
    *humi_dpct = (int16_t) (((uint16_t) frame[0] << 8) | frame[1]);
    int16_t t = (int16_t) (((uint16_t) (frame[2] & 0x7F) << 8) | frame[3]);
    if (frame[2] & 0x80) {
        t = (int16_t) (-t);
    }
    *temp_dc = t;
    return true;
}
