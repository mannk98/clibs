#ifndef CLIBS_COBS_H
#define CLIBS_COBS_H

/*
 * cobs — Consistent Overhead Byte Stuffing (stateless, no self, no struct).
 *
 * Standard COBS framing for serial/packet links. The encoded output is
 * guaranteed zero-free, so the caller may append a single 0x00 byte itself
 * as the frame delimiter — this codec writes NO trailing 0x00 delimiter
 * (the caller owns framing).
 *
 * Worst-case encoded size for an input of `len` bytes is `len + len/254 + 1`
 * (one extra overhead byte per 254-byte run plus the leading code byte).
 *
 * Freestanding portable C, no malloc, caller-provided output storage. Pure
 * functions, no global mutable state — safe to call from any context.
 */

#include <stdint.h>
#include <stddef.h>

/* bytes -> zero-free COBS (no trailing 0x00 delimiter).
 * Returns the encoded length, or 0 if out_size is too small for the
 * worst-/actual-case output / NULL data or out / len 0. On error nothing
 * is written. The output never contains a 0x00 byte. */
size_t cobs_encode(const void *data, size_t len, uint8_t *out, size_t out_size);

/* COBS -> original bytes.
 * Returns the decoded length, or 0 on a malformed stream (a code byte that
 * points past the end of the input) / out too small / NULL cobs or out /
 * len 0. On error nothing is written. */
size_t cobs_decode(const void *cobs, size_t len, uint8_t *out, size_t out_size);

#endif /* CLIBS_COBS_H */
