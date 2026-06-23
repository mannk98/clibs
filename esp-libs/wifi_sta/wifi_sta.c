#include "wifi_sta.h"
#include "backoff.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "tcpip_adapter.h"

static const char *TAG = "wifi_sta";

static wifi_sta_cb            s_cb;
static void                 *s_cb_ctx;
static backoff               s_backoff;
static volatile wifi_sta_state s_state = WIFI_STA_DISCONNECTED;
static TaskHandle_t          s_reconnect_task;

static void set_state(wifi_sta_state st) {
    s_state = st;
    if (s_cb) s_cb(st, s_cb_ctx);
}

/* Runs off its own task so it can sleep for the backoff delay. */
static void reconnect_task(void *arg) {
    (void) arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);     /* wait for a disconnect signal */
        uint32_t delay_ms = backoff_next(&s_backoff);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        set_state(WIFI_STA_CONNECTING);
        esp_wifi_connect();
    }
}

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void) arg; (void) data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        set_state(WIFI_STA_CONNECTING);
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        set_state(WIFI_STA_DISCONNECTED);
        if (s_reconnect_task) xTaskNotifyGive(s_reconnect_task);  /* schedule a spaced retry */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        backoff_reset(&s_backoff);
        set_state(WIFI_STA_CONNECTED);
        ESP_LOGI(TAG, "got IP, connected");
    }
}

esp_err_t wifi_sta_start(const wifi_sta_config *cfg) {
    if (cfg == NULL || cfg->ssid == NULL) return ESP_ERR_INVALID_ARG;

    s_cb = cfg->cb;
    s_cb_ctx = cfg->cb_ctx;
    backoff_init(&s_backoff, cfg->backoff_base_ms, cfg->backoff_max_ms);
    s_state = WIFI_STA_DISCONNECTED;

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wc = {0};
    strncpy((char *) wc.sta.ssid, cfg->ssid, sizeof(wc.sta.ssid));
    if (cfg->password)
        strncpy((char *) wc.sta.password, cfg->password, sizeof(wc.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wc));

    if (s_reconnect_task == NULL)
        xTaskCreate(reconnect_task, "wifi_sta_recon", 2048, NULL, 5, &s_reconnect_task);

    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

esp_err_t wifi_sta_stop(void) {
    esp_err_t err = esp_wifi_stop();
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
    if (s_reconnect_task) {
        vTaskDelete(s_reconnect_task);
        s_reconnect_task = NULL;
    }
    s_state = WIFI_STA_DISCONNECTED;
    return err;
}

wifi_sta_state wifi_sta_get_state(void) {
    return s_state;
}
