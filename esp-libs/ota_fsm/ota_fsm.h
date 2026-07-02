#ifndef CLIBS_OTA_FSM_H
#define CLIBS_OTA_FSM_H

#include <stdint.h>

/* Pure OTA-update state machine + download-progress tracker (C16-2).
 * Freestanding: only <stdint.h>, no SDK headers, host-testable. Caller owns
 * storage (transparent struct, no malloc). Not reentrant / not thread-safe. */

/* Lifecycle states of an OTA update. */
typedef enum {
    OTA_IDLE,        /* no update in progress */
    OTA_CHECKING,    /* a new image was announced, awaiting accept/reject */
    OTA_DOWNLOADING, /* streaming image bytes */
    OTA_VERIFYING,   /* download complete, checking integrity */
    OTA_APPLYING,    /* writing/activating the verified image */
    OTA_DONE,        /* update finished successfully (absorbing) */
    OTA_FAILED       /* update aborted/failed (absorbing) */
} ota_state;

/* Events driving the state machine. `arg` semantics (see ota_fsm_on):
 * OTA_EV_START -> arg is the total image size; OTA_EV_DATA -> arg is the
 * number of bytes just received; all other events ignore `arg`. */
typedef enum {
    OTA_EV_START,      /* begin: announce a new image of `arg` bytes */
    OTA_EV_ACCEPT,     /* accept the announced image */
    OTA_EV_REJECT,     /* reject the announced image */
    OTA_EV_DATA,       /* `arg` more bytes downloaded */
    OTA_EV_DOWNLOADED, /* all bytes received */
    OTA_EV_VERIFIED,   /* integrity check passed */
    OTA_EV_APPLIED,    /* image applied/activated */
    OTA_EV_FAIL        /* abort/failure from any active state */
} ota_event;

/* Update context. Caller allocates on the stack; fields are readable but
 * should be mutated only through the API. */
typedef struct {
    ota_state state; /* current lifecycle state */
    uint32_t total;    /* announced image size in bytes (0 until START) */
    uint32_t received; /* bytes received so far */
} ota_fsm;

/* Reset to OTA_IDLE with total=received=0. NULL self is a no-op. */
void ota_fsm_init(ota_fsm *self);

/* Read the current state without mutating. NULL self returns OTA_FAILED. */
ota_state ota_fsm_state(const ota_fsm *self);

/* Apply `ev` (with `arg`) to the machine and return the new state.
 * START records `arg` as total (received reset to 0); DATA adds `arg` to
 * received. FAIL from any active state -> OTA_FAILED. DONE/FAILED absorb all
 * events. Unexpected events leave the state unchanged. NULL self returns
 * OTA_FAILED. */
ota_state ota_fsm_on(ota_fsm *self, ota_event ev, uint32_t arg);

/* Download progress as a percentage in [0,100]: received*100/total, widened
 * through uint64_t to avoid overflow. total==0 -> 0 (divide-by-zero guard);
 * received>total -> capped at 100. NULL self returns 0. */
uint8_t ota_fsm_progress(const ota_fsm *self);

#endif /* CLIBS_OTA_FSM_H */
