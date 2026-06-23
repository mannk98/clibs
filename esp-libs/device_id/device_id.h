#ifndef CLIBS_DEVICE_ID_H
#define CLIBS_DEVICE_ID_H

#include <stdint.h>
#include <stddef.h>

/* Format the LAST 3 bytes of `mac` as lowercase hex after `prefix`, into buf[n]
 * (NUL-terminated). e.g. prefix "esp-", mac {..,0x3c,0x71,0xbf} -> "esp-3c71bf".
 * prefix NULL -> no prefix. Returns the string length (excl NUL), or -1 on
 * truncation / NULL mac|buf / n==0. Pure: no SDK headers (host-testable). */
int device_id_format(const uint8_t mac[6], char *buf, size_t n, const char *prefix);

#endif /* CLIBS_DEVICE_ID_H */
