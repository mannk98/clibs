#ifndef CLIBS_DEVICE_ID_GET_H
#define CLIBS_DEVICE_ID_GET_H

#include "esp_err.h"
#include "device_id.h"

/* Read the station MAC (esp_read_mac, ESP_MAC_WIFI_STA) and device_id_format it
 * into buf[n] with `prefix`. NULL buf / n==0 -> ESP_ERR_INVALID_ARG; a too-small
 * buffer -> ESP_ERR_INVALID_SIZE (buf set to ""). */
esp_err_t device_id_get(char *buf, size_t n, const char *prefix);

#endif /* CLIBS_DEVICE_ID_GET_H */
