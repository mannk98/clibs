#ifndef CLIBS_MQTT_NODE_H
#define CLIBS_MQTT_NODE_H

#include "esp_err.h"

/* Singleton MQTT manager for a smart-home node: connect to the gateway broker,
 * publish/subscribe, and (optionally) announce presence via a retained "online"
 * message plus a Last-Will "offline". Relies on esp-mqtt's built-in
 * auto-reconnect (no custom backoff). Mirrors wifi_sta's shape. */
typedef enum { MQTT_NODE_DISCONNECTED, MQTT_NODE_CONNECTED } mqtt_node_state;

typedef void (*mqtt_node_state_cb)(mqtt_node_state state, void *ctx);

/* Inbound message. topic/data are NOT NUL-terminated — use the given lengths. */
typedef void (*mqtt_node_msg_cb)(const char *topic, int topic_len,
                                 const char *data, int data_len, void *ctx);

/* All string pointers must stay valid for the lifetime of the connection
 * (the wrapper keeps the config by shallow copy — use literals or static storage). */
typedef struct {
    const char *uri;             /* e.g. "mqtt://192.168.4.1:1883" (required) */
    const char *client_id;       /* NULL -> esp-mqtt default */
    const char *username;        /* NULL -> no auth */
    const char *password;        /* NULL -> no auth */
    int         keepalive;       /* 0 -> SDK default (120 s) */
    /* presence (LWT) — leave presence_topic NULL to disable this whole block */
    const char *presence_topic;  /* e.g. "home/<room>/<node>/online" */
    const char *online_msg;      /* RETAINED publish on connect (e.g. "1") */
    const char *offline_msg;     /* Last-Will message (e.g. "0") */
    int         presence_qos;
    mqtt_node_state_cb state_cb; /* optional (may be NULL) */
    mqtt_node_msg_cb   msg_cb;   /* optional (may be NULL) */
    void              *cb_ctx;
} mqtt_node_config;

/* Init (once) + start the client. NULL cfg/uri -> ESP_ERR_INVALID_ARG. Idempotent. */
esp_err_t mqtt_node_start(const mqtt_node_config *cfg);

/* Stop the client (keeps it initialised for a later restart). */
esp_err_t mqtt_node_stop(void);

/* Publish. len 0 -> esp-mqtt computes strlen(data). Returns msg_id (>=0) or -1. */
int mqtt_node_publish(const char *topic, const char *data, int len, int qos, int retain);

/* Subscribe. Returns msg_id (>=0) or -1. */
int mqtt_node_subscribe(const char *topic, int qos);

mqtt_node_state mqtt_node_get_state(void);

#endif /* CLIBS_MQTT_NODE_H */
