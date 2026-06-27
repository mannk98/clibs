/*
 * base64 — RFC 4648 Base64 encode/decode (stateless).
 *
 * Standard alphabet (A-Za-z0-9+/) with '=' padding. No malloc, caller-owned
 * output buffer, no NUL terminator written. Returns chars/bytes written, or
 * 0 on empty-or-error (validated before any write, so on error the output
 * buffer is left untouched).
 */

#include "base64.h"

/* RFC 4648 standard alphabet: index -> character. */
static const char base64_enc_table[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

/* Map a base64 character to its 6-bit value, or -1 if not in the alphabet
 * (the '=' pad is also reported as -1 and handled separately by the caller). */
static int base64_dec_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

size_t base64_encode(const void *data, size_t len, char *out, size_t out_size) {
    const unsigned char *in = (const unsigned char *)data;
    size_t out_len;
    size_t i;
    size_t o = 0;

    if (data == NULL || out == NULL || len == 0) {
        return 0;
    }

    /* 4 output chars per 3-byte group, rounded up. */
    out_len = 4 * ((len + 2) / 3);
    if (out_size < out_len) {
        return 0;
    }

    /* Whole 3-byte groups. */
    for (i = 0; i + 3 <= len; i += 3) {
        uint32_t triple = ((uint32_t)in[i] << 16) |
                          ((uint32_t)in[i + 1] << 8) |
                          ((uint32_t)in[i + 2]);
        out[o++] = base64_enc_table[(triple >> 18) & 0x3F];
        out[o++] = base64_enc_table[(triple >> 12) & 0x3F];
        out[o++] = base64_enc_table[(triple >> 6) & 0x3F];
        out[o++] = base64_enc_table[triple & 0x3F];
    }

    /* Trailing 1 or 2 bytes -> '=' padding. */
    if (len - i == 1) {
        uint32_t triple = (uint32_t)in[i] << 16;
        out[o++] = base64_enc_table[(triple >> 18) & 0x3F];
        out[o++] = base64_enc_table[(triple >> 12) & 0x3F];
        out[o++] = '=';
        out[o++] = '=';
    } else if (len - i == 2) {
        uint32_t triple = ((uint32_t)in[i] << 16) | ((uint32_t)in[i + 1] << 8);
        out[o++] = base64_enc_table[(triple >> 18) & 0x3F];
        out[o++] = base64_enc_table[(triple >> 12) & 0x3F];
        out[o++] = base64_enc_table[(triple >> 6) & 0x3F];
        out[o++] = '=';
    }

    return out_len;
}

size_t base64_decode(const char *b64, size_t b64_len, void *out, size_t out_size) {
    unsigned char *dst = (unsigned char *)out;
    size_t pad = 0;
    size_t out_len;
    size_t i;
    size_t o = 0;

    if (b64 == NULL || out == NULL || b64_len == 0) {
        return 0;
    }

    /* Must be a whole number of 4-char groups. */
    if (b64_len % 4 != 0) {
        return 0;
    }

    /* Padding: only the last one or two chars may be '='. Validate every
     * non-pad char up front so a bad input writes nothing. */
    if (b64[b64_len - 1] == '=') {
        pad++;
        if (b64[b64_len - 2] == '=') {
            pad++;
        }
    }
    for (i = 0; i < b64_len - pad; i++) {
        if (base64_dec_value(b64[i]) < 0) {
            return 0;
        }
    }

    /* Decoded length: 3 bytes per group, minus the padding bytes. */
    out_len = (b64_len / 4) * 3 - pad;
    if (out_size < out_len) {
        return 0;
    }

    for (i = 0; i + 4 <= b64_len; i += 4) {
        int v0 = base64_dec_value(b64[i]);
        int v1 = base64_dec_value(b64[i + 1]);
        int v2 = base64_dec_value(b64[i + 2]);
        int v3 = base64_dec_value(b64[i + 3]);
        /* v2/v3 may be -1 only for pad chars in the final group. */
        uint32_t quad = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) |
                        ((uint32_t)(v2 < 0 ? 0 : v2) << 6) |
                        ((uint32_t)(v3 < 0 ? 0 : v3));
        dst[o++] = (unsigned char)((quad >> 16) & 0xFF);
        if (v2 >= 0) {
            dst[o++] = (unsigned char)((quad >> 8) & 0xFF);
        }
        if (v3 >= 0) {
            dst[o++] = (unsigned char)(quad & 0xFF);
        }
    }

    return out_len;
}
