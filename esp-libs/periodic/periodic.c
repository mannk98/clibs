#include "periodic.h"

esp_err_t periodic_start(periodic *self, uint32_t period_ms, periodic_cb cb, void *ctx)
{
    if (self == NULL || cb == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (self->started) {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_timer_create_args_t args = {
        .callback = cb,         /* periodic_cb has the same signature as esp_timer_cb_t */
        .arg      = ctx,
        .name     = "periodic",
    };
    esp_err_t err = esp_timer_create(&args, &self->h);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_timer_start_periodic(self->h, (uint64_t) period_ms * 1000);
    if (err != ESP_OK) {
        esp_timer_delete(self->h);
        return err;
    }
    self->started = true;
    return ESP_OK;
}

esp_err_t periodic_stop(periodic *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->started) {
        return ESP_OK;
    }
    esp_timer_stop(self->h);
    esp_timer_delete(self->h);
    self->started = false;
    return ESP_OK;
}
