#ifndef CLIBS_BASE64_H
#define CLIBS_BASE64_H

/*
 * base64 — RFC 4648 Base64 encode/decode (stateless, no self, no struct).
 *
 * Standard alphabet (A-Za-z0-9+/) with '=' padding. Freestanding portable C,
 * no malloc, caller-provided output storage. No NUL terminator is written.
 *
 * Pure functions, no global mutable state — safe to call from any context.
 */

#include <stdint.h>
#include <stddef.h>

/* bytes -> base64 (alphabet A-Za-z0-9+/, '=' padding, no NUL).
 * Returns chars written (4*ceil(len/3)), or 0 if out_size too small /
 * NULL data or out / len 0. On error nothing is written. */
size_t base64_encode(const void *data, size_t len, char *out, size_t out_size);

/* base64 -> bytes. Returns bytes written, or 0 on bad length (not a
 * multiple of 4) / invalid char / out too small / NULL / b64_len 0.
 * Handles 0/1/2 '=' padding. On error nothing is written. */
size_t base64_decode(const char *b64, size_t b64_len, void *out, size_t out_size);

#endif /* CLIBS_BASE64_H */
