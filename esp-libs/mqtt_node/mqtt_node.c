#include "mqtt_node.h"

#include "mqtt_client.h"   /* SDK esp-mqtt (no name clash: our header is mqtt_node.h) */
#include "esp_event.h"
#include "esp_log.h"

static const char *TAG = "mqtt_node";
static bool                     s_inited;
static esp_mqtt_client_handle_t s_client;
static mqtt_node_state          s_state = MQTT_NODE_DISCONNECTED;
static mqtt_node_config         s_cfg;   /* shallow copy; caller keeps strings alive */

static void event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void) arg;
    (void) base;
    esp_mqtt_event_handle_t e = (esp_mqtt_event_handle_t) event_data;

    switch ((esp_mqtt_event_id_t) event_id) {
    case MQTT_EVENT_CONNECTED:
        s_state = MQTT_NODE_CONNECTED;
        if (s_cfg.state_cb) {
            s_cfg.state_cb(MQTT_NODE_CONNECTED, s_cfg.cb_ctx);
        }
        if (s_cfg.presence_topic != NULL && s_cfg.online_msg != NULL) {
            esp_mqtt_client_publish(s_client, s_cfg.presence_topic, s_cfg.online_msg,
                                    0, s_cfg.presence_qos, 1 /* retain */);
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_state = MQTT_NODE_DISCONNECTED;
        if (s_cfg.state_cb) {
            s_cfg.state_cb(MQTT_NODE_DISCONNECTED, s_cfg.cb_ctx);
        }
        break;
    case MQTT_EVENT_DATA:
        if (s_cfg.msg_cb) {
            s_cfg.msg_cb(e->topic, e->topic_len, e->data, e->data_len, s_cfg.cb_ctx);
        }
        break;
    default:
        break;
    }
}

esp_err_t mqtt_node_start(const mqtt_node_config *cfg)
{
    if (cfg == NULL || cfg->uri == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_cfg = *cfg;   /* shallow copy of the config (incl. string pointers) */

    if (!s_inited) {
        esp_mqtt_client_config_t mc = {0};
        mc.uri = cfg->uri;
        if (cfg->client_id != NULL) mc.client_id = cfg->client_id;
        if (cfg->username  != NULL) mc.username  = cfg->username;
        if (cfg->password  != NULL) mc.password  = cfg->password;
        if (cfg->keepalive != 0)    mc.keepalive = cfg->keepalive;
        if (cfg->presence_topic != NULL) {
            mc.lwt_topic  = cfg->presence_topic;
            mc.lwt_msg    = cfg->offline_msg;
            mc.lwt_qos    = cfg->presence_qos;
            mc.lwt_retain = 1;
        }
        s_client = esp_mqtt_client_init(&mc);
        if (s_client == NULL) {
            ESP_LOGE(TAG, "esp_mqtt_client_init failed");
            return ESP_FAIL;
        }
        esp_err_t err = esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY,
                                                       event_handler, NULL);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "register_event failed: 0x%x", err);
            return err;
        }
        s_inited = true;
    }
    return esp_mqtt_client_start(s_client);
}

esp_err_t mqtt_node_stop(void)
{
    if (!s_inited || s_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    s_state = MQTT_NODE_DISCONNECTED;
    return esp_mqtt_client_stop(s_client);
}

int mqtt_node_publish(const char *topic, const char *data, int len, int qos, int retain)
{
    if (!s_inited || s_client == NULL || topic == NULL) {
        return -1;
    }
    return esp_mqtt_client_publish(s_client, topic, data, len, qos, retain);
}

int mqtt_node_subscribe(const char *topic, int qos)
{
    if (!s_inited || s_client == NULL || topic == NULL) {
        return -1;
    }
    return esp_mqtt_client_subscribe(s_client, topic, qos);
}

mqtt_node_state mqtt_node_get_state(void)
{
    return s_state;
}
