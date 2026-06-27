/* event — small publish/subscribe bus (portable, malloc-free).
 *
 * Frozen API contract (from the clibs/common OOP batch-3 design spec). The
 * struct is transparent ONLY so the caller can supply storage with no malloc;
 * its fields are private by convention — use the methods, never poke fields.
 *
 * OOP-in-C: the `event_bus` struct is the object; `event_init` is the
 * constructor; `event_subscribe`/`event_unsubscribe`/`event_publish`/
 * `event_count` are methods taking `self` first; `const event_bus *self` for
 * the non-mutating query. The only callback is an explicit user handler
 * function with `ctx` (like fsm's action) — no vtables. Every method is
 * NULL-self-safe.
 *
 * Model: a subscriber registers an (event_id, handler, ctx) triple in a
 * caller-supplied slot array. `event_publish(self, id, data)` invokes every
 * active subscriber whose event_id matches, passing (id, data, ctx), and
 * returns how many it called. Multiple subscribers per event id are allowed.
 * Not reentrant — guard with ATOMIC_BLOCK/cli if a single instance is shared
 * with an ISR.
 */
#ifndef CLIBS_EVENT_H
#define CLIBS_EVENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Explicit user handler callback (like fsm's action), not a vtable.
 * Receives the published event_id, the publisher's data pointer, and the
 * ctx supplied at subscribe time. */
typedef void (*event_handler_fn)(int event_id, void *data, void *ctx);

/* One subscription slot. Fields are private by convention. */
typedef struct {
    int              event_id;   /* event id this sub listens for (private) */
    event_handler_fn fn;         /* handler callback (private) */
    void            *ctx;        /* opaque pointer passed to fn (private) */
    bool             active;     /* slot occupied + subscribed (private) */
} event_sub;

/* The bus object. Fields are private by convention. */
typedef struct {
    event_sub *subs;             /* caller storage: cap slots (private) */
    size_t     cap;              /* total slots (private) */
    size_t     count;            /* active subscriptions (private) */
} event_bus;

/* Constructor: bind @p storage (cap slots), set count 0.
 * `self` NULL -> no-op. */
void   event_init(event_bus *self, event_sub *storage, size_t cap);

/* Register @p fn to receive @p event_id, with opaque @p ctx. Returns the
 * slot index as a subscription id (>= 0), or -1 if the table is full, fn is
 * NULL, or self is NULL. */
int    event_subscribe(event_bus *self, int event_id, event_handler_fn fn,
                       void *ctx);

/* Cancel the subscription with id @p sub_id (frees the slot, decrements
 * count). Returns true if a subscription was removed, false if the id is
 * invalid / already inactive / self is NULL. */
bool   event_unsubscribe(event_bus *self, int sub_id);

/* Publish @p event_id with @p data: invoke fn(event_id, data, ctx) for every
 * active subscriber whose event_id matches. Returns the number of handlers
 * invoked (0 if none match). Returns 0 for NULL self. */
size_t event_publish(event_bus *self, int event_id, void *data);

/* Number of active subscriptions. Returns 0 for NULL self. */
size_t event_count(const event_bus *self);

#ifdef __cplusplus
}
#endif

#endif /* CLIBS_EVENT_H */
