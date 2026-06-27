/* event — small publish/subscribe bus (portable, malloc-free).
 * Implementation of the frozen CLIBS_EVENT_H contract. Caller-storage object,
 * no heap, every method NULL-self-safe. */
#include "event.h"

void event_init(event_bus *self, event_sub *storage, size_t cap)
{
    if (self == NULL) {
        return;
    }
    self->subs = storage;
    self->cap = (storage != NULL) ? cap : 0;
    self->count = 0;
    for (size_t i = 0; i < self->cap; i++) {
        self->subs[i].event_id = 0;
        self->subs[i].fn = NULL;
        self->subs[i].ctx = NULL;
        self->subs[i].active = false;
    }
}

int event_subscribe(event_bus *self, int event_id, event_handler_fn fn,
                    void *ctx)
{
    if (self == NULL || fn == NULL) {
        return -1;
    }
    for (size_t i = 0; i < self->cap; i++) {
        if (!self->subs[i].active) {
            self->subs[i].event_id = event_id;
            self->subs[i].fn = fn;
            self->subs[i].ctx = ctx;
            self->subs[i].active = true;
            self->count++;
            return (int)i;
        }
    }
    return -1;
}

bool event_unsubscribe(event_bus *self, int sub_id)
{
    if (self == NULL || sub_id < 0 || (size_t)sub_id >= self->cap) {
        return false;
    }
    if (!self->subs[sub_id].active) {
        return false;
    }
    self->subs[sub_id].fn = NULL;
    self->subs[sub_id].ctx = NULL;
    self->subs[sub_id].active = false;
    self->count--;
    return true;
}

size_t event_publish(event_bus *self, int event_id, void *data)
{
    if (self == NULL) {
        return 0;
    }
    size_t invoked = 0;
    for (size_t i = 0; i < self->cap; i++) {
        if (self->subs[i].active && self->subs[i].event_id == event_id) {
            self->subs[i].fn(event_id, data, self->subs[i].ctx);
            invoked++;
        }
    }
    return invoked;
}

size_t event_count(const event_bus *self)
{
    if (self == NULL) {
        return 0;
    }
    return self->count;
}
