/* ota_dev — ESP8266 OTA update SDK glue. See ota_dev.h.
 *
 * COMPILE_GATE-only: this translation unit is NOT host-compiled (it pulls in the
 * ESP8266 RTOS SDK). Correctness of the pure decision logic is proven by the
 * host-tested ota_version / ota_manifest / ota_fsm suites; this file is reviewed
 * adversarially and link-checked by the xtensa COMPILE_GATE (deferred while the
 * cross-toolchain is absent). */
#include "ota_dev.h"

#include <string.h>

#include "ota_manifest.h"
#include "json_get.h"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_system.h"

/* Field-size caps for the extracted manifest strings. */
#define OTA_VERSION_MAX 16   /* "65535.65535.65535" fits in 18; 16 is enough for real versions */
#define OTA_URL_MAX     128
#define OTA_SHA_MAX     72   /* 64 hex chars + NUL, with slack */

/* Chunk buffer for streaming the image from HTTP into the OTA partition. */
#define OTA_CHUNK 1024

esp_err_t ota_init(ota *self, ota_version current, uint32_t max_size)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    self->current  = current;
    self->max_size = max_size;
    ota_fsm_init(&self->fsm);
    return ESP_OK;
}

/* Drive the FSM to OTA_FAILED and return `err` (helper for the download path). */
static esp_err_t fail(ota *self, esp_err_t err)
{
    ota_fsm_on(&self->fsm, OTA_EV_FAIL, 0);
    return err;
}

esp_err_t ota_handle_manifest(ota *self, const char *json, size_t len)
{
    if (self == NULL || json == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    (void) len; /* json is NUL-terminated by the caller; len is advisory */

    /* --- extract manifest fields ------------------------------------------ */
    char version[OTA_VERSION_MAX];
    char url[OTA_URL_MAX];
    char sha256[OTA_SHA_MAX];

    json_get_str(json, "version", version, sizeof(version), "");
    json_get_str(json, "url", url, sizeof(url), "");
    json_get_str(json, "sha256", sha256, sizeof(sha256), "");
    int size_i = json_get_int(json, "size", 0);
    uint32_t size = (size_i > 0) ? (uint32_t) size_i : 0u;

    /* --- validate + decide ------------------------------------------------- */
    if (!ota_manifest_valid(version, url, size, sha256, self->max_size)) {
        return ESP_ERR_INVALID_VERSION;
    }

    ota_version candidate;
    if (!ota_version_parse(version, &candidate)) {
        /* Redundant with ota_manifest_valid, but keeps candidate well-defined. */
        return ESP_ERR_INVALID_VERSION;
    }
    if (!ota_version_is_newer(self->current, candidate)) {
        return ESP_ERR_INVALID_VERSION; /* not newer -> nothing to do */
    }

    /* Announce the image to the FSM and accept it. */
    ota_fsm_on(&self->fsm, OTA_EV_START, size);
    ota_fsm_on(&self->fsm, OTA_EV_ACCEPT, 0);

    /* --- open the OTA partition ------------------------------------------- */
    const esp_partition_t *update_part = esp_ota_get_next_update_partition(NULL);
    if (update_part == NULL) {
        return fail(self, ESP_FAIL);
    }

    esp_ota_handle_t ota_handle = 0;
    esp_err_t err = esp_ota_begin(update_part, size, &ota_handle);
    if (err != ESP_OK) {
        return fail(self, err);
    }

    /* --- open the HTTP client -------------------------------------------- */
    esp_http_client_config_t cfg = {
        .url = url,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        esp_ota_end(ota_handle);
        return fail(self, ESP_FAIL);
    }

    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        esp_ota_end(ota_handle);
        return fail(self, err);
    }
    esp_http_client_fetch_headers(client);

    /* --- stream the image ------------------------------------------------ */
    char buf[OTA_CHUNK];
    uint32_t written = 0;
    for (;;) {
        int r = esp_http_client_read(client, buf, sizeof(buf));
        if (r < 0) {
            err = ESP_FAIL; /* transport error */
            break;
        }
        if (r == 0) {
            err = ESP_OK; /* end of stream */
            break;
        }
        err = esp_ota_write(ota_handle, buf, (size_t) r);
        if (err != ESP_OK) {
            break;
        }
        written += (uint32_t) r;
        if (written > size) {
            err = ESP_FAIL; /* body larger than announced size */
            break;
        }
        ota_fsm_on(&self->fsm, OTA_EV_DATA, (uint32_t) r);
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || written != size) {
        esp_ota_end(ota_handle);
        return fail(self, (err != ESP_OK) ? err : ESP_FAIL);
    }

    /* --- finalise + activate --------------------------------------------- */
    ota_fsm_on(&self->fsm, OTA_EV_DOWNLOADED, 0);

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        return fail(self, err);
    }
    ota_fsm_on(&self->fsm, OTA_EV_VERIFIED, 0);

    err = esp_ota_set_boot_partition(update_part);
    if (err != ESP_OK) {
        return fail(self, err);
    }
    ota_fsm_on(&self->fsm, OTA_EV_APPLIED, 0);

    /* Reboot into the new image. Does not return on success. */
    esp_restart();
    return ESP_OK; /* unreachable */
}

esp_err_t ota_mark_valid(ota *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
#if defined(CONFIG_APP_ROLLBACK_ENABLE)
    return esp_ota_mark_app_valid_cancel_rollback();
#else
    return ESP_OK; /* rollback not configured — nothing to cancel */
#endif
}
