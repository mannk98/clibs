#ifndef CLIBS_DHT_PARSE_H
#define CLIBS_DHT_PARSE_H

#include <stdint.h>
#include <stdbool.h>

/* Parse a 5-byte DHT frame. Returns true + fills outputs on a valid checksum.
 *   humidity    = (b0<<8 | b1)            -> deci-%RH  (e.g. 658 = 65.8%)
 *   temperature = ((b2&0x7F)<<8 | b3)     -> deci-C, negated if b2&0x80
 *   checksum    = (b0+b1+b2+b3) & 0xFF == b4
 * NULL arg / bad checksum -> false. Pure: no SDK headers (host-testable). */
bool dht_parse(const uint8_t frame[5], int16_t *temp_dc, int16_t *humi_dpct);

#endif /* CLIBS_DHT_PARSE_H */
