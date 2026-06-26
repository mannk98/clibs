#ifndef CLIBS_FSM_H
#define CLIBS_FSM_H

/* fsm — table-driven finite state machine (portable, malloc-free).
 *
 * OOP-in-C: the `fsm` struct is the object; `fsm_init` is the constructor;
 * `fsm_dispatch`/`fsm_state` are methods taking `self` as the first parameter.
 * The transition table is supplied by the caller and is `const` so it can live
 * in flash on an MCU. The only callback is an explicit user action with `ctx`
 * (no vtables). Every method is NULL-self-safe.
 *
 * Not reentrant — guard with ATOMIC_BLOCK/cli if a single fsm instance is
 * shared with an ISR.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One transition row. Place an array of these in flash (`static const`). */
typedef struct {
    int   from;                /* current-state id this row applies to */
    int   event;               /* event id that triggers this row */
    int   to;                  /* next-state id on a match */
    void (*action)(void *ctx); /* optional side effect; may be NULL */
} fsm_transition;

/* The state machine object. Fields are private by convention — use the
 * methods below, do not poke fields directly. */
typedef struct {
    const fsm_transition *table; /* caller-owned transition table (flash) */
    size_t                count; /* number of rows in `table` */
    int                   state; /* current state id */
    void                 *ctx;   /* opaque pointer passed to each action */
} fsm;

/* Constructor: bind the table and set the initial state.
 * `self` NULL -> no-op. */
void fsm_init(fsm *self, const fsm_transition *table, size_t count,
              int initial, void *ctx);

/* Query the current state id. Returns -1 if `self` is NULL. */
int fsm_state(const fsm *self);

/* Dispatch an event. Linear scan of the table; the FIRST row whose
 * `from == state && event == event` wins. On a match it runs `action(ctx)`
 * (if non-NULL) and THEN sets `state = to`, returning true. No matching row
 * (or NULL `self`) -> returns false and leaves the state unchanged. */
bool fsm_dispatch(fsm *self, int event);

#ifdef __cplusplus
}
#endif

#endif /* CLIBS_FSM_H */
