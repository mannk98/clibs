/* hashmap — open-addressing string->void* map over caller storage (CB3-1).
 *
 * Frozen API contract (batch-3 design spec, 2026-06-27). This header holds
 * ONLY the public contract under test: types + prototypes, no implementation.
 * Open addressing + linear probing + tombstones; FNV-1a hash of the key
 * string; values are void*; keys are stored BY POINTER, so the caller must
 * keep the key strings alive for the lifetime of the entry.
 *
 * Conventions: transparent struct = the object; constructor hashmap_init;
 * methods hashmap_<verb>(self, ...); const hashmap* for queries; no malloc
 * (caller provides the slots array); no vtables; every method NULL-self/
 * NULL-key safe. Freestanding header (<string.h> used only in the .c).
 */
#ifndef CLIBS_HASHMAP_H
#define CLIBS_HASHMAP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* One slot. Fields are private. key == NULL means an empty slot; an internal
 * tombstone marks a deleted slot so probe chains survive a remove. */
typedef struct {
    const char *key;
    void       *value;
} hashmap_entry;

/* The map object. Fields are private. */
typedef struct {
    hashmap_entry *slots;   /* caller storage: cap entries (zeroed by init) */
    size_t         cap;
    size_t         len;     /* live entries */
} hashmap;

/* Initialise the map over a caller-provided array of `cap` entries. */
void   hashmap_init(hashmap *self, hashmap_entry *storage, size_t cap);

/* Insert or update key->value. Returns false if the map is full or key is
 * NULL (or self is NULL). Updating an existing key leaves len unchanged. */
bool   hashmap_put(hashmap *self, const char *key, void *value);

/* Look up key. If found and out != NULL, *out receives the value. `out` may
 * be NULL for a pure membership test. Returns false if absent (or NULL self/
 * key); on a miss *out is left untouched. */
bool   hashmap_get(const hashmap *self, const char *key, void **out);

/* Remove key (leaves a tombstone). Returns false if absent (or NULL self/key). */
bool   hashmap_remove(hashmap *self, const char *key);

/* Number of live entries (0 if self is NULL). */
size_t hashmap_len(const hashmap *self);

/* Slot capacity (0 if self is NULL). */
size_t hashmap_cap(const hashmap *self);

#endif /* CLIBS_HASHMAP_H */
