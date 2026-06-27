#include "rotary.h"

/* Quarter-step Gray transition table, indexed by (prev_ab<<2)|ab.
 * +1 = CW, -1 = CCW, 0 = none/illegal (same state or 2-step jump). */
static const int8_t ROTARY_TABLE[16] = {
    0, 1, -1, 0,  -1, 0, 0, 1,  1, 0, 0, -1,  0, -1, 1, 0
};

void rotary_init(rotary *self, uint8_t ab) {
    if (!self) {
        return;
    }
    self->prev_ab = (uint8_t)(ab & 0x3u);
    self->position = 0;
}

rotary_dir rotary_update(rotary *self, uint8_t ab) {
    if (!self) {
        return ROTARY_NONE;
    }
    ab = (uint8_t)(ab & 0x3u);
    /* prev_ab and ab are each masked to 0..3, so the index is 0..15 and the
     * shift operates on a non-negative uint8_t value (no UB). */
    int8_t d = ROTARY_TABLE[(uint8_t)((self->prev_ab << 2) | ab)];
    self->prev_ab = ab;
    if (d > 0) {
        self->position += 1;
        return ROTARY_CW;
    }
    if (d < 0) {
        self->position -= 1;
        return ROTARY_CCW;
    }
    return ROTARY_NONE;
}

int32_t rotary_position(const rotary *self) {
    return self ? self->position : 0;
}
