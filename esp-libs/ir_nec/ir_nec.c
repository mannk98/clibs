#include "ir_nec.h"

bool ir_nec_decode(const uint16_t *us, size_t count, ir_nec_result *out)
{
    if (!us || !out || count < 66u) {
        return false;
    }

    /* Leading 9ms mark + 4.5ms space, +/-25% tolerance. */
    if (us[0] < 6750u || us[0] > 11250u) {
        return false;
    }
    if (us[1] < 3375u || us[1] > 5625u) {
        return false;
    }

    uint32_t raw = 0u;
    for (uint8_t i = 0u; i < 32u; i++) {
        /* Data space of bit i; the preceding mark (us[2 + 2i]) is ignored. */
        uint16_t space = us[3u + 2u * (uint16_t)i];
        if (space > 1124u) {                 /* 562 <-> 1687 midpoint */
            raw |= (uint32_t)1u << i;        /* LSB-first; shift of positive, no UB */
        }
    }

    uint8_t address = (uint8_t)(raw & 0xFFu);
    uint8_t command = (uint8_t)((raw >> 16) & 0xFFu);
    uint8_t cmd_inv = (uint8_t)((raw >> 24) & 0xFFu);
    if (cmd_inv != (uint8_t)~command) {      /* command complement must hold */
        return false;
    }

    out->address = address;
    out->command = command;
    out->raw = raw;
    return true;
}
