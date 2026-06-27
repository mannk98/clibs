/* hashmap — open-addressing string->void* map over caller storage (CB3-1).
 *
 * Implementation of the frozen contract in hashmap.h. Open addressing with
 * linear probing and tombstones; FNV-1a (32-bit) hash of the NUL-terminated
 * key string; values are void*; keys are stored by pointer (the caller keeps
 * the key strings alive). No malloc: all state lives in the caller-provided
 * slots array recorded by hashmap_init.
 *
 * Slot states: key == NULL -> empty (never used); key == TOMBSTONE -> deleted
 * (probe chains must continue past it). Any other key pointer is a live entry.
 */
#include "hashmap.h"

#include <string.h>     /* strcmp */

/* Distinct sentinel for a deleted slot. Its address is unique and never equal
 * to NULL nor to any real key pointer, so it cleanly distinguishes a tombstone
 * from an empty slot without stealing a value. */
static const char HASHMAP_TOMBSTONE_OBJ;
#define HASHMAP_TOMBSTONE (&HASHMAP_TOMBSTONE_OBJ)

/* FNV-1a 32-bit hash of a NUL-terminated string. Pure unsigned arithmetic —
 * no left-shift of a possibly-negative value. */
static uint32_t hashmap_hash(const char *key)
{
    uint32_t h = 2166136261u;       /* FNV offset basis */
    const unsigned char *p = (const unsigned char *)key;
    while (*p != '\0') {
        h ^= (uint32_t)*p++;
        h *= 16777619u;             /* FNV prime */
    }
    return h;
}

void hashmap_init(hashmap *self, hashmap_entry *storage, size_t cap)
{
    if (self == NULL) {
        return;
    }
    self->slots = storage;
    self->cap   = cap;
    self->len   = 0;
    if (storage != NULL) {
        for (size_t i = 0; i < cap; i++) {
            storage[i].key   = NULL;
            storage[i].value = NULL;
        }
    }
}

bool hashmap_put(hashmap *self, const char *key, void *value)
{
    if (self == NULL || self->slots == NULL || self->cap == 0 || key == NULL) {
        return false;
    }

    size_t cap   = self->cap;
    size_t start = (size_t)(hashmap_hash(key) % cap);

    /* First pass: if the key already exists, update its value in place. A live
     * slot whose key matches wins regardless of any tombstones passed. */
    size_t insert_at  = cap;        /* cap == "none found yet" */
    bool   have_slot  = false;

    for (size_t i = 0; i < cap; i++) {
        size_t idx = (start + i) % cap;
        const char *k = self->slots[idx].key;

        if (k == NULL) {
            /* Empty slot: key is not present beyond here. Remember it as an
             * insertion point only if no earlier tombstone was seen. */
            if (!have_slot) {
                insert_at = idx;
                have_slot = true;
            }
            break;
        }
        if (k == HASHMAP_TOMBSTONE) {
            /* First tombstone is the preferred reuse slot for an insert. */
            if (!have_slot) {
                insert_at = idx;
                have_slot = true;
            }
            continue;
        }
        if (strcmp(k, key) == 0) {
            self->slots[idx].value = value;     /* update; len unchanged */
            return true;
        }
    }

    if (!have_slot) {
        return false;   /* no empty/tombstone slot in cap probes -> full */
    }

    self->slots[insert_at].key   = key;
    self->slots[insert_at].value = value;
    self->len++;
    return true;
}

bool hashmap_get(const hashmap *self, const char *key, void **out)
{
    if (self == NULL || self->slots == NULL || self->cap == 0 || key == NULL) {
        return false;
    }

    size_t cap   = self->cap;
    size_t start = (size_t)(hashmap_hash(key) % cap);

    for (size_t i = 0; i < cap; i++) {
        size_t idx = (start + i) % cap;
        const char *k = self->slots[idx].key;

        if (k == NULL) {
            return false;       /* empty slot ends the probe chain */
        }
        if (k == HASHMAP_TOMBSTONE) {
            continue;           /* skip deleted slots */
        }
        if (strcmp(k, key) == 0) {
            if (out != NULL) {
                *out = self->slots[idx].value;
            }
            return true;
        }
    }
    return false;
}

bool hashmap_remove(hashmap *self, const char *key)
{
    if (self == NULL || self->slots == NULL || self->cap == 0 || key == NULL) {
        return false;
    }

    size_t cap   = self->cap;
    size_t start = (size_t)(hashmap_hash(key) % cap);

    for (size_t i = 0; i < cap; i++) {
        size_t idx = (start + i) % cap;
        const char *k = self->slots[idx].key;

        if (k == NULL) {
            return false;       /* empty slot ends the probe chain */
        }
        if (k == HASHMAP_TOMBSTONE) {
            continue;
        }
        if (strcmp(k, key) == 0) {
            self->slots[idx].key   = HASHMAP_TOMBSTONE;   /* leave a tombstone */
            self->slots[idx].value = NULL;
            self->len--;
            return true;
        }
    }
    return false;
}

size_t hashmap_len(const hashmap *self)
{
    return (self == NULL) ? 0 : self->len;
}

size_t hashmap_cap(const hashmap *self)
{
    return (self == NULL) ? 0 : self->cap;
}
