#include "esp_log.h"
#include "wifi_sta.h"
#include "backoff.h"
#include "debounce.h"
#include "filter.h"
#include "mqtt_topic.h"
#include "nvs_kv.h"
#include "mqtt_node.h"
#include "device_id_get.h"

static const char *TAG = "esp_libs_gate";

void app_main(void)
{
    /* This project exists only to cross-compile + link esp-libs for the ESP8266
       target (a compile-gate). It is never expected to run on hardware. We touch
       a symbol from each lib so the linker pulls them in. */

    backoff b;
    backoff_init(&b, 500, 30000);
    ESP_LOGI(TAG, "backoff first=%u", (unsigned) backoff_next(&b));

    debounce d;
    debounce_init(&d, 3, 0);
    (void) debounce_update(&d, 1);

    ema f;
    ema_init(&f, 3, 0);
    (void) ema_update(&f, 100);

    char topic[32];
    (void) mqtt_topic_join(topic, sizeof topic, (const char *const[]){"home", "node", "state"}, 3);
    (void) mqtt_topic_match("home/#", topic);

    wifi_sta_config cfg = {
        .ssid = "gateway-ap", .password = "secret",
        .backoff_base_ms = 500, .backoff_max_ms = 30000,
        .cb = 0, .cb_ctx = 0,
    };
    (void) wifi_sta_start(&cfg);
    ESP_LOGI(TAG, "state=%d", (int) wifi_sta_get_state());

    /* nvs_kv compile-gate: touch enough of the surface to link it. */
    (void) nvs_kv_init();
    nvs_kv kv;
    if (nvs_kv_open(&kv, "node", true) == ESP_OK) {
        char ssid[33];
        (void) nvs_kv_get_str(&kv, "ssid", ssid, sizeof ssid, "default-ap");
        (void) nvs_kv_set_u32(&kv, "boot", 1);
        (void) nvs_kv_commit(&kv);
        nvs_kv_close(&kv);
    }

    /* mqtt_node compile-gate: touch enough of the surface to link it. */
    mqtt_node_config mcfg = {
        .uri = "mqtt://192.168.4.1:1883",
        .client_id = "node-1",
        .presence_topic = "home/livingroom/node-1/online",
        .online_msg = "1", .offline_msg = "0", .presence_qos = 1,
        .state_cb = 0, .msg_cb = 0, .cb_ctx = 0,
    };
    (void) mqtt_node_start(&mcfg);
    (void) mqtt_node_subscribe("home/livingroom/node-1/set", 1);
    (void) mqtt_node_publish("home/livingroom/node-1/state", "on", 0, 1, 1);
    ESP_LOGI(TAG, "mqtt state=%d", (int) mqtt_node_get_state());

    /* device_id compile-gate. */
    char id[24];
    (void) device_id_get(id, sizeof id, "esp-");
    ESP_LOGI(TAG, "id=%s", id);
}
