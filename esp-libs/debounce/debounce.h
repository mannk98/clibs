#ifndef CLIBS_DEBOUNCE_H
#define CLIBS_DEBOUNCE_H

#include <stdint.h>
#include <stdbool.h>

/* Digital input debounce. Caller samples the raw level at a fixed interval and
 * feeds it to debounce_update; a change is committed only after `threshold`
 * consecutive samples of the new level. Not thread-safe. */
typedef enum { DEBOUNCE_NONE, DEBOUNCE_RISING, DEBOUNCE_FALLING } debounce_edge;

typedef struct {
    uint8_t stable;     /* committed debounced level (0/1) */
    uint8_t candidate;  /* level currently being counted */
    uint8_t count;      /* consecutive samples of candidate */
    uint8_t threshold;  /* samples needed to commit a change (>=1) */
} debounce;

/* threshold 0 is clamped to 1; stable starts at (initial ? 1 : 0). */
void          debounce_init(debounce *self, uint8_t threshold, uint8_t initial);
/* Feed one sampled level; returns the edge when a change is committed, else NONE. */
debounce_edge debounce_update(debounce *self, uint8_t raw);
/* Current committed level (0/1). */
uint8_t       debounce_level(const debounce *self);

#endif /* CLIBS_DEBOUNCE_H */
