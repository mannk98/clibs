#include "wifi_sta.h"

/* STUB (cycle-2 Task 1): establishes the build/link gate. Real STA logic in Task 2. */
static wifi_sta_state s_state = WIFI_STA_DISCONNECTED;

esp_err_t wifi_sta_start(const wifi_sta_config *cfg) {
    if (cfg == NULL || cfg->ssid == NULL) return ESP_ERR_INVALID_ARG;
    return ESP_OK;
}

esp_err_t wifi_sta_stop(void) {
    s_state = WIFI_STA_DISCONNECTED;
    return ESP_OK;
}

wifi_sta_state wifi_sta_get_state(void) {
    return s_state;
}
