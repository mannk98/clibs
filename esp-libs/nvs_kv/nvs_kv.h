#ifndef CLIBS_NVS_KV_H
#define CLIBS_NVS_KV_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "nvs.h"   /* for nvs_handle_t */

/* Reusable key/value store over ONE NVS namespace. Caller owns the `nvs_kv`
 * storage (no malloc). Transparent struct + self-methods (clibs convention). */
typedef struct {
    nvs_handle_t h;
    bool         is_open;
} nvs_kv;

/* Initialise the NVS flash partition once (idempotent). On NO_FREE_PAGES /
 * NEW_VERSION_FOUND it erases the partition and retries. Returns ESP_OK on
 * success. Call once at boot before nvs_kv_open(). */
esp_err_t nvs_kv_init(void);

/* Open a namespace. writable=true -> NVS_READWRITE, false -> NVS_READONLY.
 * NULL self/ns -> ESP_ERR_INVALID_ARG. */
esp_err_t nvs_kv_open(nvs_kv *self, const char *ns, bool writable);

/* Close the namespace handle (no-op if not open / self NULL). */
void nvs_kv_close(nvs_kv *self);

/* Read a string into buf[n] (always NUL-terminated on ESP_OK). Missing key ->
 * copies `def` (NULL treated as "") into buf and returns ESP_OK. A stored value
 * that would not fit in n -> buf set to "" and ESP_ERR_NVS_INVALID_LENGTH.
 * NULL self/key/buf or n==0 -> ESP_ERR_INVALID_ARG. Not open -> ESP_ERR_INVALID_STATE. */
esp_err_t nvs_kv_get_str(nvs_kv *self, const char *key, char *buf, size_t n, const char *def);

/* Write a string value (pending until nvs_kv_commit). */
esp_err_t nvs_kv_set_str(nvs_kv *self, const char *key, const char *val);

/* Read a u32. Missing key -> *out = def, returns ESP_OK. */
esp_err_t nvs_kv_get_u32(nvs_kv *self, const char *key, uint32_t *out, uint32_t def);

/* Write a u32 value (pending until nvs_kv_commit). */
esp_err_t nvs_kv_set_u32(nvs_kv *self, const char *key, uint32_t val);

/* Flush pending writes to flash. */
esp_err_t nvs_kv_commit(nvs_kv *self);

#endif /* CLIBS_NVS_KV_H */
