#include "device_id_get.h"

#include "esp_system.h"   /* esp_read_mac, ESP_MAC_WIFI_STA */

esp_err_t device_id_get(char *buf, size_t n, const char *prefix)
{
    if (buf == NULL || n == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t mac[6];
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        buf[0] = '\0';
        return err;
    }
    if (device_id_format(mac, buf, n, prefix) < 0) {
        buf[0] = '\0';
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}
