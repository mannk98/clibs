/* tlv — Type-Length-Value writer + reader objects (freestanding, no malloc).
 *
 * Record layout: [type:1][vlen:1][value:vlen]. See tlv.h for the contract.
 * <string.h> is used only here (memcpy) per the freestanding convention.
 */

#include "tlv.h"

#include <string.h>

void tlv_writer_init(tlv_writer *self, void *buf, size_t cap) {
    if (self == NULL) {
        return;
    }
    self->buf = (uint8_t *)buf;
    self->cap = cap;
    self->len = 0;
}

bool tlv_write(tlv_writer *self, uint8_t type, const void *value, uint8_t vlen) {
    if (self == NULL) {
        return false;
    }
    /* A record needs 2 header bytes ([type][vlen]) plus vlen value bytes. */
    size_t need = (size_t)2 + (size_t)vlen;
    if (need > self->cap - self->len) {
        return false; /* no room — buffer and len left unchanged */
    }
    self->buf[self->len]     = type;
    self->buf[self->len + 1] = vlen;
    if (vlen != 0) {
        memcpy(&self->buf[self->len + 2], value, vlen);
    }
    self->len += need;
    return true;
}

size_t tlv_writer_len(const tlv_writer *self) {
    if (self == NULL) {
        return 0;
    }
    return self->len;
}

void tlv_reader_init(tlv_reader *self, const void *buf, size_t len) {
    if (self == NULL) {
        return;
    }
    self->buf = (const uint8_t *)buf;
    self->len = len;
    self->pos = 0;
}

bool tlv_next(tlv_reader *self, uint8_t *type, const uint8_t **value, uint8_t *vlen) {
    if (self == NULL) {
        return false;
    }
    /* Need the 2-byte header at pos. */
    if (self->pos + 2 > self->len) {
        return false; /* end of buffer */
    }
    uint8_t t = self->buf[self->pos];
    uint8_t vl = self->buf[self->pos + 1];
    /* The claimed value must fit within the buffer. */
    if (self->pos + 2 + (size_t)vl > self->len) {
        return false; /* truncated record */
    }
    *type = t;
    *value = &self->buf[self->pos + 2];
    *vlen = vl;
    self->pos += (size_t)2 + (size_t)vl;
    return true;
}

bool tlv_find(const tlv_reader *self, uint8_t type, const uint8_t **value, uint8_t *vlen) {
    if (self == NULL) {
        return false;
    }
    /* Independent scan from the start; does not touch the reader cursor. */
    size_t scan = 0;
    while (scan + 2 <= self->len) {
        uint8_t t = self->buf[scan];
        uint8_t vl = self->buf[scan + 1];
        if (scan + 2 + (size_t)vl > self->len) {
            return false; /* truncated — stop scanning */
        }
        if (t == type) {
            *value = &self->buf[scan + 2];
            *vlen = vl;
            return true;
        }
        scan += (size_t)2 + (size_t)vl;
    }
    return false;
}
