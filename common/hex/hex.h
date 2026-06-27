#ifndef CLIBS_HEX_H
#define CLIBS_HEX_H

/* hex - stateless hex encode/decode (no malloc, no object/self).
 *
 * Frozen API contract from the batch-2 design spec
 * (docs/superpowers/specs/2026-06-27-clibs-common-oop-batch2-design.md).
 * This header declares the contract only; the implementation lives in
 * hex.c (written in the GREEN step). Freestanding: depends only on
 * <stdint.h>/<stddef.h>.
 *
 * Return contract for both functions: number of chars/bytes written;
 * 0 = empty-or-error. No NUL terminator is ever written.
 */

#include <stdint.h>
#include <stddef.h>

/* bytes -> lowercase hex (0-9a-f), no NUL written.
 * Returns chars written (len*2), or 0 if out_size < len*2, NULL data/out,
 * or len 0. */
size_t hex_encode(const void *data, size_t len, char *out, size_t out_size);

/* hex -> bytes. Accepts upper- and lower-case hex digits.
 * Returns bytes written (hex_len/2), or 0 on odd hex_len, any non-hex
 * char, out too small, NULL hex/out, or hex_len 0. Writes nothing on error. */
size_t hex_decode(const char *hex, size_t hex_len, void *out, size_t out_size);

#endif /* CLIBS_HEX_H */
