/* scheduler — cooperative tick-driven task scheduler (portable, malloc-free).
 *
 * Frozen API contract (from the clibs/common OOP batch-3 design spec). The
 * struct is transparent ONLY so the caller can supply storage with no malloc;
 * its fields are private by convention — use the methods, never poke fields.
 *
 * OOP-in-C: the `scheduler` struct is the object; `scheduler_init` is the
 * constructor; `scheduler_add`/`scheduler_remove`/`scheduler_tick`/
 * `scheduler_count` are methods taking `self` first. The only callback is an
 * explicit user task function with `ctx` (like fsm's action) — no vtables.
 * Every method is NULL-self-safe.
 *
 * Model: `scheduler_tick()` increments an internal tick counter `now`, then
 * runs every active task whose `next_due` has been reached, advancing each by
 * its `period` for a steady cadence (period must be >= 1). Not reentrant —
 * guard with ATOMIC_BLOCK/cli if a single instance is shared with an ISR.
 */
#ifndef CLIBS_SCHEDULER_H
#define CLIBS_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Explicit user task callback (like fsm's action), not a vtable. */
typedef void (*scheduler_task_fn)(void *ctx);

/* One task slot. Fields are private by convention. */
typedef struct {
    scheduler_task_fn fn;        /* task callback (private) */
    void             *ctx;       /* opaque pointer passed to fn (private) */
    uint32_t          period;    /* ticks between runs, >= 1 (private) */
    uint32_t          next_due;  /* tick at which it next runs (private) */
    bool              active;    /* slot occupied + running (private) */
} scheduler_task;

/* The scheduler object. Fields are private; supply storage via scheduler_init. */
typedef struct {
    scheduler_task *tasks;       /* caller storage: cap slots (private) */
    size_t          cap;         /* capacity in slots (private) */
    size_t          count;       /* active tasks (private) */
    uint32_t        now;         /* current tick (private) */
} scheduler;

/* Constructor: bind @p storage (cap slots), set count 0 and now 0.
 * `self` NULL -> no-op. */
void   scheduler_init(scheduler *self, scheduler_task *storage, size_t cap);

/* Register a task running every @p period ticks (period >= 1). Sets
 * next_due = now + period and returns the slot index as a task id (>= 0).
 * Returns -1 if the table is full, period is 0, fn is NULL, or self is NULL. */
int    scheduler_add(scheduler *self, scheduler_task_fn fn, void *ctx,
                     uint32_t period);

/* Deactivate the task with id @p task_id (frees the slot, decrements count).
 * Returns true if a task was removed, false if the id is invalid / inactive /
 * self is NULL. */
bool   scheduler_remove(scheduler *self, int task_id);

/* Advance one tick: now++, then run fn(ctx) for every active task whose
 * next_due is reached, advancing each by its period (steady cadence).
 * `self` NULL -> no-op. */
void   scheduler_tick(scheduler *self);

/* Number of active tasks. Returns 0 for NULL self. */
size_t scheduler_count(const scheduler *self);

#ifdef __cplusplus
}
#endif

#endif /* CLIBS_SCHEDULER_H */
