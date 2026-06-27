#ifndef CLIBS_IR_NEC_H
#define CLIBS_IR_NEC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Decoded NEC frame. `raw` is the full 32-bit payload (addr/~addr/cmd/~cmd, in
 * bit order so that address = raw & 0xFF and command = (raw>>16) & 0xFF). It is
 * exposed verbatim so extended-NEC remotes (16-bit address) still round-trip. */
typedef struct {
    uint8_t  address;
    uint8_t  command;
    uint32_t raw;
} ir_nec_result;

/* Decode a NEC frame from mark/space durations in microseconds:
 *   us[0] = leading mark  (~9000), us[1] = leading space (~4500),
 *   then 32 (mark, space) pairs -> requires count >= 66.
 * Header tolerance +/-25% (mark 6750..11250, space 3375..5625). Each data space
 * is classified 0 (~562) or 1 (~1687) by the 1124us midpoint; the marks are
 * ignored. Bits are assembled LSB-first into `raw`; address = raw & 0xFF,
 * command = (raw>>16) & 0xFF, and the command complement
 * ((raw>>24)&0xFF == (uint8_t)~command) is validated.
 * Returns true and fills *out on a valid frame; returns false on bad header,
 * count < 66, failed complement, or NULL us/out. Pure, host-testable.
 * Not reentrant with respect to a shared *out. */
bool ir_nec_decode(const uint16_t *us, size_t count, ir_nec_result *out);

#endif /* CLIBS_IR_NEC_H */
