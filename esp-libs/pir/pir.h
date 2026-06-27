#ifndef CLIBS_PIR_H
#define CLIBS_PIR_H

#include <stdint.h>
#include <stdbool.h>

/* Event emitted by pir_update on the FSM edges. */
typedef enum { PIR_NONE, PIR_MOTION_START, PIR_MOTION_END } pir_event;

/* Software retrigger/hold for a PIR sensor. Fields private. Not reentrant. */
typedef struct { uint32_t hold_ms; uint32_t last_ms; bool active; } pir;

/* Initialise the FSM idle with the given hold time (ms). No-op if self is NULL. */
void pir_init(pir *self, uint32_t hold_ms);

/* Feed the raw sensor level + a monotonic now_ms. motion_raw refreshes last_ms; on
 * the rising edge from idle returns PIR_MOTION_START. With no motion and active and
 * (now-last) >= hold_ms returns PIR_MOTION_END (and goes idle). Else PIR_NONE.
 * NULL self -> PIR_NONE. now compared via wrap-safe unsigned subtraction. */
pir_event pir_update(pir *self, bool motion_raw, uint32_t now_ms);

/* Current active state; false if self is NULL. */
bool pir_active(const pir *self);

#endif /* CLIBS_PIR_H */
