#ifndef CLIBS_BACKOFF_H
#define CLIBS_BACKOFF_H

#include <stdint.h>

/* Exponential reconnect backoff (e.g. for WiFi/MQTT). Not thread-safe. */
typedef struct {
    uint32_t base_ms;
    uint32_t max_ms;
    uint32_t cur_ms;
} backoff;

/* Initialize: cur = base; if max < base, max is raised to base. */
void backoff_init(backoff *self, uint32_t base_ms, uint32_t max_ms);
/* Return the current delay, then double it toward max_ms (overflow-safe cap). */
uint32_t backoff_next(backoff *self);
/* Reset cur back to base (call on a successful connect). */
void backoff_reset(backoff *self);

#endif /* CLIBS_BACKOFF_H */
