/* bitset — fixed-capacity bit array over caller-provided uint32_t[] storage.
 *
 * Frozen API contract (from the clibs/common OOP batch-1 design spec). The
 * struct is transparent ONLY so the caller can supply storage with no malloc;
 * its fields are private by convention — use the methods, never poke fields.
 * Every method tolerates a NULL self (no-op / safe default).
 *
 * Not reentrant — guard with ATOMIC_BLOCK/cli if shared with an ISR.
 */
#ifndef CLIBS_BITSET_H
#define CLIBS_BITSET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** The bitset object. Fields are private; supply storage via bitset_init. */
typedef struct {
    uint32_t *words;   /* caller storage: BITSET_WORDS(nbits) words (private) */
    size_t    nbits;   /* capacity in bits (private) */
} bitset;

/** Number of uint32_t words needed to hold @p nbits bits (size the caller's array). */
#define BITSET_WORDS(nbits) (((nbits) + 31u) / 32u)

/** Constructor: bind @p words (length BITSET_WORDS(nbits)) and zero all bits. */
void   bitset_init(bitset *self, uint32_t *words, size_t nbits);

/** Set bit @p i to 1. Out-of-range index or NULL self: no-op. */
void   bitset_set(bitset *self, size_t i);

/** Clear bit @p i to 0. Out-of-range index or NULL self: no-op. */
void   bitset_clear(bitset *self, size_t i);

/** Flip bit @p i. Out-of-range index or NULL self: no-op. */
void   bitset_toggle(bitset *self, size_t i);

/** Query bit @p i. Returns false for out-of-range index or NULL self. */
bool   bitset_test(const bitset *self, size_t i);

/** Set exactly the first nbits bits (masks the final partial word). NULL self: no-op. */
void   bitset_set_all(bitset *self);

/** Clear all bits. NULL self: no-op. */
void   bitset_clear_all(bitset *self);

/** Popcount of set bits. Returns 0 for NULL self. */
size_t bitset_count(const bitset *self);

/** Lowest set bit index -> *out (written only on true). Returns false if none / NULL self. */
bool   bitset_find_first_set(const bitset *self, size_t *out);

#endif /* CLIBS_BITSET_H */
