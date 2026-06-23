#ifndef CLIBS_PERIODIC_H
#define CLIBS_PERIODIC_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_timer.h"   /* for esp_timer_handle_t */

/* Periodic-callback helper over esp_timer. Caller owns the `periodic` storage. */
typedef struct {
    esp_timer_handle_t h;
    bool               started;
} periodic;

/* Runs in the esp_timer task — keep it short and non-blocking. */
typedef void (*periodic_cb)(void *ctx);

/* Create + start a timer firing every period_ms. NULL self|cb -> ESP_ERR_INVALID_ARG;
 * already started -> ESP_ERR_INVALID_STATE. */
esp_err_t periodic_start(periodic *self, uint32_t period_ms, periodic_cb cb, void *ctx);

/* Stop + delete the timer. NULL self -> ESP_ERR_INVALID_ARG; not started -> ESP_OK (no-op). */
esp_err_t periodic_stop(periodic *self);

#endif /* CLIBS_PERIODIC_H */
