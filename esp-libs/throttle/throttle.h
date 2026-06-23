#ifndef CLIBS_THROTTLE_H
#define CLIBS_THROTTLE_H

#include <stdint.h>
#include <stdbool.h>

/* Minimum-interval gate (e.g. rate-limit telemetry publishes). The caller supplies
 * the current time in ms (from millis / esp_timer / the periodic helper). Not thread-safe. */
typedef struct {
    uint32_t interval_ms;
    uint32_t last_ms;
    bool     primed;
} throttle;

void throttle_init(throttle *self, uint32_t interval_ms);
/* First call -> true (primes). Otherwise true (and updates) once interval_ms has
 * elapsed since the last allow; false meanwhile. Wrap-safe unsigned subtraction. */
bool throttle_allow(throttle *self, uint32_t now_ms);

#endif /* CLIBS_THROTTLE_H */
