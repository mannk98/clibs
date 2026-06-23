#include "esp_log.h"
#include "wifi_sta.h"
#include "backoff.h"
#include "debounce.h"
#include "filter.h"
#include "mqtt_topic.h"

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
}
