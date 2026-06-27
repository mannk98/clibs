#ifndef CLIBS_SLIP_H
#define CLIBS_SLIP_H

/*
 * slip — SLIP serial framing (RFC 1055) codec (stateless, no self, no struct).
 *
 * RFC 1055 constants:
 *   END     = 0xC0  (frame delimiter)
 *   ESC     = 0xDB  (escape)
 *   ESC_END = 0xDC  (escaped END:  ESC ESC_END  -> END)
 *   ESC_ESC = 0xDD  (escaped ESC:  ESC ESC_ESC  -> ESC)
 *
 * Encode: each payload END byte -> ESC ESC_END; each ESC byte -> ESC ESC_ESC;
 * every other byte verbatim; then one trailing END byte is appended (the
 * trailing-END contract). Worst-case encoded size = 2*len + 1.
 *
 * Decode: walk the frame; ESC followed by ESC_END -> END, ESC followed by
 * ESC_ESC -> ESC, ESC followed by anything else -> malformed (0); an unescaped
 * END terminates the frame; reaching len with no terminating END -> malformed (0).
 *
 * Freestanding portable C, no malloc, caller-provided output storage. Pure
 * functions, no global mutable state — safe to call from any context.
 */

#include <stdint.h>
#include <stddef.h>

/* bytes -> escaped payload + a trailing END byte.
 * Returns the encoded length, or 0 if out_size is too small for the actual
 * output / NULL data or out / len 0. On error nothing is written. */
size_t slip_encode(const void *data, size_t len, uint8_t *out, size_t out_size);

/* SLIP frame -> original bytes. Unescapes up to (and consuming) the first
 * unescaped END byte.
 * Returns the decoded payload length, or 0 on a malformed frame (bad escape /
 * no terminating END found) / out too small / NULL slip or out / len 0. On
 * error nothing is written. */
size_t slip_decode(const void *slip, size_t len, uint8_t *out, size_t out_size);

#endif /* CLIBS_SLIP_H */
