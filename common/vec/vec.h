/* vec — generic dynamic array + stack over a caller-provided byte buffer.
 *
 * Frozen API contract (from the clibs/common OOP batch-2 design spec). The
 * struct is transparent ONLY so the caller can supply storage with no malloc;
 * its fields are private by convention — use the methods, never poke fields.
 * Value semantics: every push/pop/get/set copies exactly elem_size bytes.
 * vec_push/vec_pop give LIFO stack use; vec_get/vec_set give random access.
 * Every method tolerates a NULL self (no-op / safe default).
 */
#ifndef CLIBS_VEC_H
#define CLIBS_VEC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/** The vec object. Fields are private; supply storage via vec_init. */
typedef struct {
    uint8_t *buf;        /* caller storage: cap*elem_size bytes (private) */
    size_t   elem_size;  /* size of one element in bytes (private) */
    size_t   cap;        /* capacity in elements (private) */
    size_t   len;        /* current element count (private) */
} vec;

/** Bytes the caller's storage buffer must hold for @p cap elements. */
#define VEC_BYTES(elem_size, cap) ((elem_size) * (cap))

/** Constructor: bind @p storage (cap*elem_size bytes), set len 0. */
void   vec_init(vec *self, void *storage, size_t cap, size_t elem_size);

/** Append @p elem (copy elem_size bytes). Returns false if full / NULL self. */
bool   vec_push(vec *self, const void *elem);

/** Remove last element into @p out (out may be NULL). Returns false if empty / NULL self. */
bool   vec_pop(vec *self, void *out);

/** Copy element @p i to @p out. Returns false if i>=len / NULL self. */
bool   vec_get(const vec *self, size_t i, void *out);

/** Overwrite element @p i with @p elem. Returns false if i>=len / NULL self. */
bool   vec_set(vec *self, size_t i, const void *elem);

/** Current element count. Returns 0 for NULL self. */
size_t vec_len(const vec *self);

/** Capacity in elements. Returns 0 for NULL self. */
size_t vec_cap(const vec *self);

/** True if len == 0. Returns true for NULL self. */
bool   vec_is_empty(const vec *self);

/** True if len == cap. Returns false for NULL self. */
bool   vec_is_full(const vec *self);

/** Reset to empty (len = 0); storage bytes untouched. NULL self: no-op. */
void   vec_clear(vec *self);

#endif /* CLIBS_VEC_H */
