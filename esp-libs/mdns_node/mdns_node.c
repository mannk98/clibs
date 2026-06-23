#include "mdns_node.h"

#include <string.h>

#include "lwip/ip4_addr.h"   /* ip4_addr_t, ip4addr_ntoa */
#include "esp_err.h"

/* Forward-declare only the three mDNS symbols we use, avoiding the full mdns.h
 * which unconditionally declares mdns_query_aaaa(ip6_addr_t*) — ip6_addr_t is
 * not defined when CONFIG_LWIP_IPV6 is off (ESP8266 RTOS SDK v3.4 defconfig). */
esp_err_t mdns_init(void);
esp_err_t mdns_hostname_set(const char *hostname);
esp_err_t mdns_query_a(const char *host_name, uint32_t timeout, ip4_addr_t *addr);

esp_err_t mdns_node_init(const char *hostname)
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        return err;
    }
    if (hostname != NULL) {
        err = mdns_hostname_set(hostname);
    }
    return err;
}

esp_err_t mdns_node_resolve(const char *host, char *ip_buf, size_t n, uint32_t timeout_ms)
{
    if (host == NULL || ip_buf == NULL || n == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    ip_buf[0] = '\0';
    ip4_addr_t addr;
    memset(&addr, 0, sizeof addr);
    esp_err_t err = mdns_query_a(host, timeout_ms, &addr);
    if (err != ESP_OK) {
        return err;
    }
    const char *s = ip4addr_ntoa(&addr);   /* static buffer — copy immediately */
    size_t len = strlen(s);
    if (len >= n) {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(ip_buf, s, len + 1);
    return ESP_OK;
}
