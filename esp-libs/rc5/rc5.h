#ifndef CLIBS_RC5_H
#define CLIBS_RC5_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Decoded RC5 frame. `raw` is the full 14-bit payload (S1 S2 T A4..A0 C5..C0,
 * MSB-first) exposed verbatim. The convenience fields are derived from it:
 * toggle = (raw>>11)&1, address = (raw>>6)&0x1F, command = raw&0x3F. */
typedef struct {
    uint8_t  toggle;
    uint8_t  address;
    uint8_t  command;
    uint16_t raw;
} rc5_result;

/* Decode an RC5 frame from 28 Manchester half-bit samples (each 0 or 1):
 *   bit i is the pair (halfbits[2i], halfbits[2i+1]);
 *   (0,1) = logical 1, (1,0) = logical 0, anything else -> invalid -> false.
 * The 14 bits are assembled MSB-first into a 14-bit `raw`
 * (S1 S2 T A4..A0 C5..C0). The leading start bit S1 = (raw>>13)&1 must be 1.
 * On success fills *out: address = (raw>>6)&0x1F, command = raw&0x3F,
 * toggle = (raw>>11)&1, raw = raw.
 * Returns false on NULL halfbits/out, count != 28, an invalid Manchester pair,
 * or S1 == 0. Pure, host-testable. Not reentrant with respect to a shared *out. */
bool rc5_decode(const uint8_t *halfbits, size_t count, rc5_result *out);

#endif /* CLIBS_RC5_H */
