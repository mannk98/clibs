#include "device_id.h"

#include <string.h>

static char hex_digit(uint8_t nib)
{
    return (nib < 10) ? (char) ('0' + nib) : (char) ('a' + nib - 10);
}

int device_id_format(const uint8_t mac[6], char *buf, size_t n, const char *prefix)
{
    if (mac == NULL || buf == NULL || n == 0) {
        return -1;
    }
    size_t plen = (prefix != NULL) ? strlen(prefix) : 0;
    if (plen + 6 + 1 > n) {       /* prefix + 6 hex chars + NUL */
        buf[0] = '\0';
        return -1;
    }
    size_t i = 0;
    for (size_t p = 0; p < plen; p++) {
        buf[i++] = prefix[p];
    }
    for (size_t b = 3; b < 6; b++) {            /* last 3 MAC bytes */
        buf[i++] = hex_digit((uint8_t) (mac[b] >> 4));
        buf[i++] = hex_digit((uint8_t) (mac[b] & 0x0F));
    }
    buf[i] = '\0';
    return (int) i;
}
