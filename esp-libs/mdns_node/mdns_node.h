#ifndef CLIBS_MDNS_NODE_H
#define CLIBS_MDNS_NODE_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* Initialise mDNS and (optionally) set the hostname. hostname NULL -> just init.
 * (Named mdns_node to avoid shadowing the SDK header mdns.h.) */
esp_err_t mdns_node_init(const char *hostname);

/* Resolve `host` (A record) to a dotted-quad string in ip_buf[n]. timeout in ms.
 * NULL host|ip_buf or n==0 -> ESP_ERR_INVALID_ARG; on lookup failure returns the
 * SDK error with ip_buf set to ""; result too long -> ESP_ERR_INVALID_SIZE. */
esp_err_t mdns_node_resolve(const char *host, char *ip_buf, size_t n, uint32_t timeout_ms);

#endif /* CLIBS_MDNS_NODE_H */
