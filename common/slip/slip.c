/*
 * slip — SLIP serial framing (RFC 1055) codec implementation.
 *
 * Stateless, no malloc, caller-provided output storage. Each function does a
 * sizing/validation pass first, then only writes to `out` once the full output
 * is known to fit — so a too-small or malformed input writes nothing.
 */

#include "slip.h"

/* RFC 1055 framing constants. */
#define SLIP_END     0xC0u /* frame delimiter */
#define SLIP_ESC     0xDBu /* escape */
#define SLIP_ESC_END 0xDCu /* ESC ESC_END -> END */
#define SLIP_ESC_ESC 0xDDu /* ESC ESC_ESC -> ESC */

size_t slip_encode(const void *data, size_t len, uint8_t *out, size_t out_size)
{
    const uint8_t *in;
    size_t i;
    size_t need;

    if (data == NULL || out == NULL || len == 0) {
        return 0;
    }

    in = (const uint8_t *)data;

    /* Sizing pass: every byte is 1, END/ESC become 2, plus a trailing END. */
    need = 1; /* trailing END */
    for (i = 0; i < len; i++) {
        if (in[i] == SLIP_END || in[i] == SLIP_ESC) {
            need += 2;
        } else {
            need += 1;
        }
    }

    if (need > out_size) {
        return 0; /* too small: write nothing */
    }

    /* Write pass. */
    {
        size_t w = 0;
        for (i = 0; i < len; i++) {
            if (in[i] == SLIP_END) {
                out[w++] = SLIP_ESC;
                out[w++] = SLIP_ESC_END;
            } else if (in[i] == SLIP_ESC) {
                out[w++] = SLIP_ESC;
                out[w++] = SLIP_ESC_ESC;
            } else {
                out[w++] = in[i];
            }
        }
        out[w++] = SLIP_END; /* trailing END */
        return w;
    }
}

size_t slip_decode(const void *slip, size_t len, uint8_t *out, size_t out_size)
{
    const uint8_t *in;
    size_t i;
    size_t need;
    int saw_end;

    if (slip == NULL || out == NULL || len == 0) {
        return 0;
    }

    in = (const uint8_t *)slip;

    /* Validation + sizing pass: decode logically without writing. */
    need = 0;
    saw_end = 0;
    for (i = 0; i < len; i++) {
        uint8_t b = in[i];
        if (b == SLIP_END) {
            saw_end = 1; /* unescaped END terminates the frame */
            break;
        }
        if (b == SLIP_ESC) {
            uint8_t nb;
            if (i + 1 >= len) {
                return 0; /* dangling ESC -> malformed */
            }
            nb = in[i + 1];
            if (nb != SLIP_ESC_END && nb != SLIP_ESC_ESC) {
                return 0; /* bad escape -> malformed */
            }
            i++; /* consume the escaped byte */
        }
        need += 1;
    }

    if (!saw_end) {
        return 0; /* no terminating END -> malformed */
    }
    if (need > out_size) {
        return 0; /* too small: write nothing */
    }

    /* Write pass: replay the now-validated decode into `out`. */
    {
        size_t w = 0;
        for (i = 0; i < len; i++) {
            uint8_t b = in[i];
            if (b == SLIP_END) {
                break;
            }
            if (b == SLIP_ESC) {
                uint8_t nb = in[i + 1];
                out[w++] = (nb == SLIP_ESC_END) ? (uint8_t)SLIP_END
                                                : (uint8_t)SLIP_ESC;
                i++;
            } else {
                out[w++] = b;
            }
        }
        return w;
    }
}
