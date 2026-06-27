/* scheduler — cooperative tick-driven task scheduler (portable, malloc-free).
 *
 * Minimal implementation of the frozen CLIBS_SCHEDULER_H contract. All state
 * lives in the caller-provided `scheduler` + `scheduler_task` storage; no heap.
 * Every method is NULL-self-safe and treats the struct fields as private.
 *
 * Cadence model (steady): `scheduler_tick()` does now++ then, for each active
 * task whose next_due has been reached, calls fn(ctx) and advances
 * next_due += period. A task added with period P and now N first fires at
 * tick N+P and then every P ticks thereafter (period must be >= 1).
 */
#include "scheduler.h"

void scheduler_init(scheduler *self, scheduler_task *storage, size_t cap) {
    if (self == NULL) {
        return;
    }
    self->tasks = storage;
    self->cap = (storage == NULL) ? 0u : cap;
    self->count = 0u;
    self->now = 0u;

    for (size_t i = 0; i < self->cap; i++) {
        self->tasks[i].fn = NULL;
        self->tasks[i].ctx = NULL;
        self->tasks[i].period = 0u;
        self->tasks[i].next_due = 0u;
        self->tasks[i].active = false;
    }
}

int scheduler_add(scheduler *self, scheduler_task_fn fn, void *ctx,
                  uint32_t period) {
    if (self == NULL || fn == NULL || period == 0u) {
        return -1;
    }

    for (size_t i = 0; i < self->cap; i++) {
        if (!self->tasks[i].active) {
            self->tasks[i].fn = fn;
            self->tasks[i].ctx = ctx;
            self->tasks[i].period = period;
            self->tasks[i].next_due = self->now + period;
            self->tasks[i].active = true;
            self->count++;
            return (int)i;
        }
    }

    return -1; /* table full */
}

bool scheduler_remove(scheduler *self, int task_id) {
    if (self == NULL || task_id < 0 || (size_t)task_id >= self->cap) {
        return false;
    }
    if (!self->tasks[task_id].active) {
        return false;
    }

    self->tasks[task_id].active = false;
    self->tasks[task_id].fn = NULL;
    self->tasks[task_id].ctx = NULL;
    self->tasks[task_id].period = 0u;
    self->tasks[task_id].next_due = 0u;
    self->count--;
    return true;
}

void scheduler_tick(scheduler *self) {
    if (self == NULL) {
        return;
    }

    self->now++;

    for (size_t i = 0; i < self->cap; i++) {
        scheduler_task *t = &self->tasks[i];
        if (t->active && self->now >= t->next_due) {
            t->next_due += t->period; /* steady cadence */
            t->fn(t->ctx);
        }
    }
}

size_t scheduler_count(const scheduler *self) {
    if (self == NULL) {
        return 0u;
    }
    return self->count;
}
