#include "stepper.h"

/* Half-step coil patterns indexed by phase 0..7. uint8_t so all coil-pattern
 * operands stay positive (no left-shift of a possibly-negative value). */
static const uint8_t STEPPER_HALF[8] = {
    0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09
};

void stepper_init(stepper *self)
{
    if (self == NULL) {
        return;
    }
    self->phase = 0;
    self->position = 0;
}

uint8_t stepper_step(stepper *self, bool forward)
{
    if (self == NULL) {
        return 0;
    }
    self->phase = (uint8_t) ((self->phase + (forward ? 1u : 7u)) & 7u);
    self->position += forward ? 1 : -1;
    return STEPPER_HALF[self->phase];
}

uint8_t stepper_coils(const stepper *self)
{
    if (self == NULL) {
        return 0;
    }
    return STEPPER_HALF[self->phase & 7u];
}

int32_t stepper_position(const stepper *self)
{
    if (self == NULL) {
        return 0;
    }
    return self->position;
}
