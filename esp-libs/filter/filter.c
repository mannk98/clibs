#include "filter.h"
#include <stddef.h>

void ema_init(ema *self, uint8_t shift, int32_t initial) {
    if (self == NULL) return;
    self->shift = shift;
    self->acc   = (int32_t)((uint32_t)initial << shift); /* well-defined for the documented range */
}

int32_t ema_update(ema *self, int32_t sample) {
    if (self == NULL) return 0;
    self->acc += sample - (self->acc >> self->shift);
    return self->acc >> self->shift;
}
