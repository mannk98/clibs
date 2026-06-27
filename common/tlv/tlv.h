#ifndef CLIBS_TLV_H
#define CLIBS_TLV_H

/* tlv — Type-Length-Value writer + reader objects (freestanding, no malloc).
 *
 * Record layout on the wire: [type:1][vlen:1][value:vlen]. A 1-byte length
 * bounds each value to 0..255 bytes. Two transparent-struct objects:
 *   - tlv_writer: append records into a caller-provided buffer.
 *   - tlv_reader: iterate (tlv_next) or scan (tlv_find) records in a buffer.
 *
 * Conventions: caller owns all storage; methods are self-first; NULL self is
 * safe (no-op / defined failure value). Out params of tlv_next / tlv_find must
 * be non-NULL. No vtables, no dynamic allocation.
 *
 * NOTE (RED step): this header is the frozen API contract under test. The
 * implementation (tlv.c) does not exist yet — linking test_tlv.c without it
 * yields the expected "undefined symbols" RED.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* TLV writer: appends [type][vlen][value] records into buf (fields private). */
typedef struct {
    uint8_t *buf;   /* caller-provided destination buffer */
    size_t   cap;   /* total capacity of buf in bytes */
    size_t   len;   /* bytes written so far */
} tlv_writer;

/* Initialise a writer over buf[cap]. len becomes 0. NULL self is a no-op. */
void tlv_writer_init(tlv_writer *self, void *buf, size_t cap);

/* Append one record [type][vlen][value]. Needs 2+vlen free bytes; returns
 * false (buffer and len unchanged) if it does not fit or self is NULL.
 * value may be NULL only when vlen is 0. Returns true on success. */
bool tlv_write(tlv_writer *self, uint8_t type, const void *value, uint8_t vlen);

/* Bytes written so far. Returns 0 if self is NULL. */
size_t tlv_writer_len(const tlv_writer *self);

/* TLV reader: iterates / scans records in buf (fields private). */
typedef struct {
    const uint8_t *buf;   /* source buffer (read-only) */
    size_t         len;   /* number of valid bytes in buf */
    size_t         pos;   /* cursor of the next record for tlv_next */
} tlv_reader;

/* Initialise a reader over buf[len]. pos becomes 0. NULL self is a no-op. */
void tlv_reader_init(tlv_reader *self, const void *buf, size_t len);

/* Read the record at the cursor and advance. Sets *type, *value (pointer into
 * buf), *vlen and returns true; returns false at end-of-buffer or on a
 * truncated record (claimed vlen overruns the buffer), or if self is NULL.
 * type, value and vlen must be non-NULL. */
bool tlv_next(tlv_reader *self, uint8_t *type, const uint8_t **value, uint8_t *vlen);

/* Scan from the start for the first record whose type matches; on success sets
 * *value and *vlen and returns true. Independent of the reader cursor (takes a
 * const reader; pos is not modified). Returns false if absent or self is NULL.
 * value and vlen must be non-NULL. */
bool tlv_find(const tlv_reader *self, uint8_t type, const uint8_t **value, uint8_t *vlen);

#endif /* CLIBS_TLV_H */
