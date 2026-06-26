/* bitset — implementation. See bitset.h for the contract.
 *
 * Word layout: bit i lives in words[i >> 5] at offset (i & 31). The final
 * partial word (when nbits is not a multiple of 32) is masked by set_all so
 * out-of-range bits stay 0 and never inflate the popcount. Every public
 * function NULL-guards self and bounds-checks the index.
 */
#include "bitset.h"

void bitset_init(bitset *self, uint32_t *words, size_t nbits) {
    if (!self) return;
    self->words = words;
    self->nbits = nbits;
    bitset_clear_all(self);
}

void bitset_set(bitset *self, size_t i) {
    if (!self || i >= self->nbits) return;
    self->words[i >> 5] |= (uint32_t)1u << (i & 31u);
}

void bitset_clear(bitset *self, size_t i) {
    if (!self || i >= self->nbits) return;
    self->words[i >> 5] &= ~((uint32_t)1u << (i & 31u));
}

void bitset_toggle(bitset *self, size_t i) {
    if (!self || i >= self->nbits) return;
    self->words[i >> 5] ^= (uint32_t)1u << (i & 31u);
}

bool bitset_test(const bitset *self, size_t i) {
    if (!self || i >= self->nbits) return false;
    return ((self->words[i >> 5] >> (i & 31u)) & 1u) != 0u;
}

void bitset_clear_all(bitset *self) {
    if (!self) return;
    size_t nwords = BITSET_WORDS(self->nbits);
    for (size_t w = 0; w < nwords; w++) self->words[w] = 0u;
}

void bitset_set_all(bitset *self) {
    if (!self || self->nbits == 0) return;
    size_t nwords = BITSET_WORDS(self->nbits);
    for (size_t w = 0; w < nwords; w++) self->words[w] = 0xFFFFFFFFu;
    size_t rem = self->nbits & 31u;                 /* mask the final partial word */
    if (rem) self->words[nwords - 1] = ((uint32_t)1u << rem) - 1u;
}

size_t bitset_count(const bitset *self) {
    if (!self) return 0;
    size_t nwords = BITSET_WORDS(self->nbits), total = 0;
    for (size_t w = 0; w < nwords; w++) {
        uint32_t x = self->words[w];
        while (x) { x &= x - 1u; total++; }         /* Kernighan popcount */
    }
    return total;
}

bool bitset_find_first_set(const bitset *self, size_t *out) {
    if (!self) return false;
    size_t nwords = BITSET_WORDS(self->nbits);
    for (size_t w = 0; w < nwords; w++) {
        uint32_t x = self->words[w];
        if (x) {
            size_t bit = 0;
            while ((x & 1u) == 0u) { x >>= 1; bit++; }
            size_t idx = w * 32u + bit;
            if (idx >= self->nbits) return false;
            if (out) *out = idx;
            return true;
        }
    }
    return false;
}
