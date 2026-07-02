#include "ota_fsm.h"

#include <stddef.h> /* NULL */

void ota_fsm_init(ota_fsm *self) {
    if (self == NULL) {
        return;
    }
    self->state = OTA_IDLE;
    self->total = 0;
    self->received = 0;
}

ota_state ota_fsm_state(const ota_fsm *self) {
    if (self == NULL) {
        return OTA_FAILED;
    }
    return self->state;
}

ota_state ota_fsm_on(ota_fsm *self, ota_event ev, uint32_t arg) {
    if (self == NULL) {
        return OTA_FAILED;
    }

    /* Terminal states absorb every event. */
    if (self->state == OTA_DONE || self->state == OTA_FAILED) {
        return self->state;
    }

    /* FAIL aborts from any active (non-terminal) state. */
    if (ev == OTA_EV_FAIL) {
        self->state = OTA_FAILED;
        return self->state;
    }

    switch (self->state) {
    case OTA_IDLE:
        if (ev == OTA_EV_START) {
            self->total = arg;
            self->received = 0;
            self->state = OTA_CHECKING;
        }
        break;
    case OTA_CHECKING:
        if (ev == OTA_EV_ACCEPT) {
            self->state = OTA_DOWNLOADING;
        } else if (ev == OTA_EV_REJECT) {
            self->state = OTA_IDLE;
        }
        break;
    case OTA_DOWNLOADING:
        if (ev == OTA_EV_DATA) {
            self->received += arg;
        } else if (ev == OTA_EV_DOWNLOADED) {
            self->state = OTA_VERIFYING;
        }
        break;
    case OTA_VERIFYING:
        if (ev == OTA_EV_VERIFIED) {
            self->state = OTA_APPLYING;
        }
        break;
    case OTA_APPLYING:
        if (ev == OTA_EV_APPLIED) {
            self->state = OTA_DONE;
        }
        break;
    default:
        /* OTA_DONE / OTA_FAILED handled above; nothing else to do. */
        break;
    }

    return self->state;
}

uint8_t ota_fsm_progress(const ota_fsm *self) {
    if (self == NULL || self->total == 0) {
        return 0;
    }
    uint64_t pct = (uint64_t)self->received * 100u / self->total;
    if (pct > 100u) {
        pct = 100u;
    }
    return (uint8_t)pct;
}
