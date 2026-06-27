/* hex - stateless hex encode/decode (no malloc, no object/self).
 *
 * Implements the frozen API contract declared in hex.h. Freestanding:
 * depends only on <stdint.h>/<stddef.h> (declared via the header).
 *
 * Return contract for both functions: number of chars/bytes written;
 * 0 = empty-or-error. No NUL terminator is ever written, and nothing is
 * written to the output buffer on the error path.
 */

#include "hex.h"

static const char HEX_LOWER[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

/* Map a single hex character to its 0..15 value, or -1 if not a hex digit.
 * Accepts upper- and lower-case. */
static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

size_t hex_encode(const void *data, size_t len, char *out, size_t out_size) {
    const uint8_t *bytes = (const uint8_t *)data;
    size_t need;
    size_t i;

    if (data == NULL || out == NULL || len == 0) {
        return 0;
    }
    need = len * 2u;
    if (out_size < need) {
        return 0;
    }
    for (i = 0; i < len; i++) {
        uint8_t b = bytes[i];
        out[i * 2u]      = HEX_LOWER[(b >> 4) & 0x0Fu];
        out[i * 2u + 1u] = HEX_LOWER[b & 0x0Fu];
    }
    return need;
}

size_t hex_decode(const char *hex, size_t hex_len, void *out, size_t out_size) {
    uint8_t *bytes = (uint8_t *)out;
    size_t need;
    size_t i;

    if (hex == NULL || out == NULL || hex_len == 0) {
        return 0;
    }
    if ((hex_len % 2u) != 0u) {
        return 0;
    }
    need = hex_len / 2u;
    if (out_size < need) {
        return 0;
    }
    /* Validate every nibble before writing anything, so the error path
     * leaves the output buffer untouched. */
    for (i = 0; i < hex_len; i++) {
        if (hex_nibble(hex[i]) < 0) {
            return 0;
        }
    }
    for (i = 0; i < need; i++) {
        int hi = hex_nibble(hex[i * 2u]);
        int lo = hex_nibble(hex[i * 2u + 1u]);
        bytes[i] = (uint8_t)((hi << 4) | lo);
    }
    return need;
}
