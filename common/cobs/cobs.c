#include "cobs.h"

/*
 * Standard COBS (Consistent Overhead Byte Stuffing).
 *
 * Encode: split the input into runs delimited by 0x00. Each run of up to 253
 * non-zero bytes is prefixed by a code byte = (run length + 1); a code byte of
 * 0xFF marks a full 254-byte run that was NOT terminated by a zero (the next
 * block continues without consuming a zero). Every original 0x00 is replaced by
 * a block boundary, so the output never contains a 0x00 byte. No trailing
 * delimiter is written — the caller owns framing.
 *
 * Decode: read a code byte, copy (code - 1) literal bytes, then (unless code was
 * 0xFF) emit one 0x00 for the implicit terminator — except after the final block.
 *
 * No malloc, no global state; the caller owns both buffers. All index/pointer
 * math is bounds-checked against out_size / len so a malformed stream or an
 * undersized output buffer returns 0 with nothing written past the buffer.
 */

size_t cobs_encode(const void *data, size_t len, uint8_t *out, size_t out_size)
{
    const uint8_t *in = (const uint8_t *)data;
    size_t out_idx;      /* next free slot in out */
    size_t code_idx;     /* slot reserved for the current block's code byte */
    uint8_t code;        /* 1 + number of literal bytes copied so far in block */
    size_t i;

    if (in == NULL || out == NULL || len == 0) {
        return 0;
    }

    /* Reserve the first code byte. */
    if (out_size < 1) {
        return 0;
    }
    code_idx = 0;
    out_idx = 1;
    code = 1;

    for (i = 0; i < len; i++) {
        if (in[i] == 0x00) {
            /* Close the current block: write its code, open a new one. */
            out[code_idx] = code;
            if (out_idx >= out_size) {
                return 0;
            }
            code_idx = out_idx;
            out_idx++;
            code = 1;
        } else {
            if (out_idx >= out_size) {
                return 0;
            }
            out[out_idx] = in[i];
            out_idx++;
            code++;
            if (code == 0xFF) {
                /* Full 254-byte run not terminated by a zero: emit code and
                 * start a fresh block that does NOT consume a zero. */
                out[code_idx] = code;
                if (i + 1 < len) {
                    if (out_idx >= out_size) {
                        return 0;
                    }
                    code_idx = out_idx;
                    out_idx++;
                    code = 1;
                }
            }
        }
    }

    /* Write the final block's code byte (unless it was already flushed by a
     * trailing 0xFF run that ended exactly on the last input byte). */
    if (code != 0xFF) {
        out[code_idx] = code;
    }

    return out_idx;
}

size_t cobs_decode(const void *cobs, size_t len, uint8_t *out, size_t out_size)
{
    const uint8_t *in = (const uint8_t *)cobs;
    size_t in_idx;
    size_t out_idx;

    if (in == NULL || out == NULL || len == 0) {
        return 0;
    }

    in_idx = 0;
    out_idx = 0;

    while (in_idx < len) {
        uint8_t code = in[in_idx];
        uint8_t k;

        if (code == 0x00) {
            /* A 0x00 code byte is never valid in a COBS stream. */
            return 0;
        }
        in_idx++;

        /* code - 1 literal bytes follow this code byte. */
        for (k = 1; k < code; k++) {
            if (in_idx >= len) {
                /* Code byte claims more literals than the input holds. */
                return 0;
            }
            if (out_idx >= out_size) {
                return 0;
            }
            out[out_idx] = in[in_idx];
            out_idx++;
            in_idx++;
        }

        /* A non-0xFF block that is not the last block had an implicit zero. */
        if (code != 0xFF && in_idx < len) {
            if (out_idx >= out_size) {
                return 0;
            }
            out[out_idx] = 0x00;
            out_idx++;
        }
    }

    return out_idx;
}
