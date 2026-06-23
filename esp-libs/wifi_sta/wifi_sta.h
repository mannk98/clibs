#ifndef CLIBS_WIFI_STA_H
#define CLIBS_WIFI_STA_H

#include <stdint.h>
#include "esp_err.h"

/* STA-mode WiFi manager: connect to an AP and reconnect with exponential backoff. */
typedef enum { WIFI_STA_DISCONNECTED, WIFI_STA_CONNECTING, WIFI_STA_CONNECTED } wifi_sta_state;
typedef void (*wifi_sta_cb)(wifi_sta_state state, void *ctx);

typedef struct {
    const char  *ssid;
    const char  *password;        /* may be NULL for open AP */
    uint32_t     backoff_base_ms; /* reconnect backoff base, e.g. 500 */
    uint32_t     backoff_max_ms;  /* reconnect backoff cap,  e.g. 30000 */
    wifi_sta_cb  cb;              /* optional status callback (may be NULL) */
    void        *cb_ctx;
} wifi_sta_config;

esp_err_t      wifi_sta_start(const wifi_sta_config *cfg);
esp_err_t      wifi_sta_stop(void);
wifi_sta_state wifi_sta_get_state(void);

#endif /* CLIBS_WIFI_STA_H */
