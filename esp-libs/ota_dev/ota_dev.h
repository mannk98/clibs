#ifndef CLIBS_OTA_DEV_H
#define CLIBS_OTA_DEV_H

#include "esp_err.h"
#include "ota_version.h"
#include "ota_fsm.h"

/* ota_dev — ESP8266 OTA update SDK glue (L3 glue part).
 *
 * SDK glue — COMPILE_GATE-only, NOT host-compiled (cross-toolchain deferred).
 * The pure decision logic lives in the host-tested parts:
 *   - ota_version  (parse/compare semver)
 *   - ota_manifest (ota_manifest_valid: field validation)
 *   - ota_fsm      (lifecycle state machine + download progress)
 *
 * ota_dev binds those together with the ESP8266 RTOS SDK: it fetches a manifest,
 * validates it, decides whether the announced image is newer, then streams the
 * image over HTTP into the OTA partition via esp_ota_ops, and reboots.
 *
 * Conventions: no malloc, caller-owned transparent struct + self-methods,
 * NULL-guarded. Not reentrant / not thread-safe (single update at a time). */

typedef struct {
    ota_version current;  /* version currently running (for is-newer decision) */
    ota_fsm     fsm;      /* update lifecycle + download progress */
    uint32_t    max_size; /* reject manifests announcing more than this many bytes */
} ota;

/* Initialise: record the running version and the max acceptable image size, and
 * reset the FSM to OTA_IDLE. ESP_OK / ESP_ERR_INVALID_ARG (NULL self). */
esp_err_t ota_init(ota *self, ota_version current, uint32_t max_size);

/* Handle an announced manifest (JSON with string fields "version", "url",
 * "sha256" and integer "size").
 *
 * Steps: extract fields -> ota_manifest_valid -> ota_version_is_newer -> drive
 * the FSM through CHECKING/ACCEPT -> esp_ota_begin -> esp_http_client GET the
 * image, esp_ota_write each chunk (feeding OTA_EV_DATA to the FSM) -> esp_ota_end
 * -> esp_ota_set_boot_partition -> esp_restart (does not return on success).
 *
 * Returns ESP_ERR_INVALID_ARG (NULL self/json), ESP_ERR_INVALID_VERSION
 * (manifest invalid or not newer), or a propagated SDK error on download/flash
 * failure (with the FSM driven to OTA_FAILED). Not reentrant. */
esp_err_t ota_handle_manifest(ota *self, const char *json, size_t len);

/* Cancel the rollback watchdog after a freshly-applied image has booted and
 * self-tested OK, marking the running app valid so it is not rolled back on the
 * next reset. ESP_OK / ESP_ERR_INVALID_ARG (NULL self) / propagated SDK error. */
esp_err_t ota_mark_valid(ota *self);

#endif /* CLIBS_OTA_DEV_H */
