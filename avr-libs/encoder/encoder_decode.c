#include "encoder_decode.h"

void encoder_decode(uint8_t *pre_a, uint8_t *count, uint8_t a, uint8_t b, int *counter) {
    if (a != *pre_a) {
        (*count)++;
        if (*count == 2) {
            if (a == b) (*counter)++;
            else        (*counter)--;
            *count = 0;
        }
        *pre_a = a;
    }
}
