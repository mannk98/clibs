#include "nvs_kv.h"

#include <string.h>

#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "nvs_kv";
static bool s_inited;

esp_err_t nvs_kv_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs erase + reinit (err=0x%x)", err);
        err = nvs_flash_erase();
        if (err == ESP_OK) {
            err = nvs_flash_init();
        }
    }
    if (err == ESP_OK) {
        s_inited = true;
    } else {
        ESP_LOGE(TAG, "nvs_flash_init failed: 0x%x", err);
    }
    return err;
}

esp_err_t nvs_kv_open(nvs_kv *self, const char *ns, bool writable)
{
    if (self == NULL || ns == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->is_open = false;
    esp_err_t err = nvs_open(ns, writable ? NVS_READWRITE : NVS_READONLY, &self->h);
    if (err == ESP_OK) {
        self->is_open = true;
    }
    return err;
}

void nvs_kv_close(nvs_kv *self)
{
    if (self != NULL && self->is_open) {
        nvs_close(self->h);
        self->is_open = false;
    }
}

/* Copy `def` (NULL -> "") into buf[n]; INVALID_LENGTH if it would not fit. */
static esp_err_t copy_default(char *buf, size_t n, const char *def)
{
    if (def == NULL) {
        buf[0] = '\0';
        return ESP_OK;
    }
    size_t len = strlen(def);
    if (len >= n) {
        buf[0] = '\0';
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    memcpy(buf, def, len + 1);
    return ESP_OK;
}

esp_err_t nvs_kv_get_str(nvs_kv *self, const char *key, char *buf, size_t n, const char *def)
{
    if (self == NULL || key == NULL || buf == NULL || n == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    size_t required = 0;
    esp_err_t err = nvs_get_str(self->h, key, NULL, &required);  /* probe size (incl NUL) */
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return copy_default(buf, n, def);
    }
    if (err != ESP_OK) {
        buf[0] = '\0';
        return err;
    }
    if (required > n) {
        buf[0] = '\0';
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    size_t cap = n;
    return nvs_get_str(self->h, key, buf, &cap);
}

esp_err_t nvs_kv_set_str(nvs_kv *self, const char *key, const char *val)
{
    if (self == NULL || key == NULL || val == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    return nvs_set_str(self->h, key, val);
}

esp_err_t nvs_kv_get_u32(nvs_kv *self, const char *key, uint32_t *out, uint32_t def)
{
    if (self == NULL || key == NULL || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = nvs_get_u32(self->h, key, out);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *out = def;
        return ESP_OK;
    }
    return err;
}

esp_err_t nvs_kv_set_u32(nvs_kv *self, const char *key, uint32_t val)
{
    if (self == NULL || key == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    return nvs_set_u32(self->h, key, val);
}

esp_err_t nvs_kv_commit(nvs_kv *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->is_open) {
        return ESP_ERR_INVALID_STATE;
    }
    return nvs_commit(self->h);
}
