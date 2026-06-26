/* fsm — table-driven finite state machine (portable, malloc-free).
 *
 * Implementation of the CLIBS_FSM_H contract. All state lives in the
 * caller-provided `fsm` struct; this file owns no heap memory and every
 * public method is NULL-self-safe. */

#include "fsm.h"

/* Constructor: bind the table and set the initial state.
 * `self` NULL -> no-op. */
void fsm_init(fsm *self, const fsm_transition *table, size_t count,
              int initial, void *ctx)
{
    if (self == NULL) {
        return;
    }
    self->table = table;
    self->count = count;
    self->state = initial;
    self->ctx   = ctx;
}

/* Query the current state id. Returns -1 if `self` is NULL. */
int fsm_state(const fsm *self)
{
    if (self == NULL) {
        return -1;
    }
    return self->state;
}

/* Dispatch an event. Linear scan of the table; the FIRST row whose
 * `from == state && event == event` wins. On a match it runs `action(ctx)`
 * (if non-NULL) and THEN sets `state = to`, returning true. No matching row
 * (or NULL `self`) -> returns false and leaves the state unchanged. */
bool fsm_dispatch(fsm *self, int event)
{
    size_t i;

    if (self == NULL || self->table == NULL) {
        return false;
    }

    for (i = 0; i < self->count; i++) {
        const fsm_transition *row = &self->table[i];
        if (row->from == self->state && row->event == event) {
            if (row->action != NULL) {
                row->action(self->ctx); /* action runs BEFORE the state update */
            }
            self->state = row->to;
            return true;
        }
    }

    return false; /* no matching row: state unchanged */
}
