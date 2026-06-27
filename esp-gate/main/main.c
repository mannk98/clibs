#include "esp_log.h"
#include "wifi_sta.h"
#include "backoff.h"
#include "debounce.h"
#include "filter.h"
#include "mqtt_topic.h"
#include "nvs_kv.h"
#include "mqtt_node.h"
#include "device_id_get.h"
#include "json_build.h"
#include "json_get.h"
#include "periodic.h"
#include "mdns_node.h"
#include "relay.h"
#include "button.h"
#include "dht.h"
#include "pwm_dimmer.h"
#include "hysteresis.h"
#include "crc.h"
#include "map_range.h"
#include "median.h"
#include "throttle.h"
#include "adc_a0.h"
#include "i2c_bus.h"
#include "spi_bus.h"
#include "ds18b20.h"
#include "servo.h"
#include "hcsr04.h"
#include "saturating_counter.h"
#include "bme280.h"
#include "ws2812.h"
#include "ir_nec_rx.h"
#include "pir_gpio.h"
#include "ssd1306.h"
#include "ssd1306_text.h"
#include "max7219.h"
#include "rotary_gpio.h"
#include "stepper_gpio.h"
#include "hx711_gpio.h"
#include "rc5_rx.h"

static const char *TAG = "esp_libs_gate";

static void gate_tick(void *ctx) { (void) ctx; }

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

    /* json compile-gate. */
    char payload[64];
    json_build jb;
    json_build_init(&jb, payload, sizeof payload);
    json_build_str(&jb, "state", "on");
    json_build_int(&jb, "rssi", -60);
    (void) json_build_end(&jb);
    char cmd[16];
    (void) json_get_str("{\"cmd\":\"on\"}", "cmd", cmd, sizeof cmd, "off");
    (void) json_get_int("{\"level\":42}", "level", 0);
    ESP_LOGI(TAG, "payload=%s cmd=%s", payload, cmd);

    /* periodic compile-gate. */
    periodic pt;
    (void) periodic_start(&pt, 5000, gate_tick, 0);
    (void) periodic_stop(&pt);

    /* mdns_node compile-gate. */
    (void) mdns_node_init("node-1");
    char bip[16];
    (void) mdns_node_resolve("broker.local", bip, sizeof bip, 1000);
    ESP_LOGI(TAG, "broker ip=%s", bip);

    /* relay compile-gate. */
    relay r;
    (void) relay_init(&r, GPIO_NUM_5, false);
    (void) relay_set(&r, true);
    (void) relay_toggle(&r);
    ESP_LOGI(TAG, "relay on=%d", (int) relay_is_on(&r));

    /* button compile-gate. */
    button btn;
    (void) button_init(&btn, GPIO_NUM_4, true, 3);
    (void) button_poll(&btn);
    ESP_LOGI(TAG, "btn pressed=%d", (int) button_is_pressed(&btn));

    /* dht compile-gate. */
    dht dh;
    (void) dht_init(&dh, GPIO_NUM_2);
    int16_t dht_t = 0, dht_h = 0;
    (void) dht_read(&dh, &dht_t, &dht_h);
    ESP_LOGI(TAG, "dht t=%d h=%d", (int) dht_t, (int) dht_h);

    /* pwm_dimmer compile-gate. */
    pwm_dimmer dim;
    (void) pwm_dimmer_init(&dim, GPIO_NUM_14, 1000);
    (void) pwm_dimmer_set(&dim, 50);
    ESP_LOGI(TAG, "dimmer set");

    /* L1 extras compile-gate. */
    hysteresis hy;
    hysteresis_init(&hy, 20, 25, false);
    (void) hysteresis_update(&hy, 22);
    uint8_t crcbuf[3] = {1, 2, 3};
    (void) crc8_maxim(crcbuf, 3);
    (void) crc16_modbus(crcbuf, 3);
    (void) map_range(512, 0, 1023, 0, 100);
    (void) clamp_i32(150, 0, 100);
    (void) median3(1, 2, 3);
    throttle th;
    throttle_init(&th, 1000);
    (void) throttle_allow(&th, 0);
    ESP_LOGI(TAG, "l1 extras linked");

    /* adc_a0 compile-gate. */
    (void) adc_a0_init();
    uint16_t araw = 0;
    (void) adc_a0_read(&araw);
    ema af;
    ema_init(&af, 3, 0);
    int32_t aval = 0;
    (void) adc_a0_read_ema(&af, &aval);
    ESP_LOGI(TAG, "adc raw=%u smoothed=%d", (unsigned) araw, (int) aval);

    /* i2c_bus compile-gate. */
    (void) i2c_bus_init(GPIO_NUM_4, GPIO_NUM_5);
    uint8_t i2cbuf[2] = {0};
    (void) i2c_bus_write_reg(0x76, 0xF4, i2cbuf, 1);
    (void) i2c_bus_read_reg(0x76, 0xFA, i2cbuf, 2);
    ESP_LOGI(TAG, "i2c reg0=%u", (unsigned) i2cbuf[0]);

    /* spi_bus compile-gate: raw bus transfer + a software-CS spi_device. */
    (void) spi_bus_init();
    uint8_t spitx[4] = {0x9F, 0, 0, 0};
    uint8_t spirx[4] = {0};
    (void) spi_bus_transfer(spitx, spirx, 4);
    spi_device spidev;
    (void) spi_device_init(&spidev, GPIO_NUM_15);
    (void) spi_device_transfer(&spidev, spitx, spirx, 4);
    ESP_LOGI(TAG, "spi id0=%u", (unsigned) spirx[1]);

    /* L3 sensors compile-gate. */
    ds18b20 ow;
    (void) ds18b20_init(&ow, GPIO_NUM_2);
    int32_t owt = 0;
    (void) ds18b20_read(&ow, &owt);
    servo sv;
    (void) servo_init(&sv, GPIO_NUM_14);
    (void) servo_set(&sv, 90);
    hcsr04 us;
    (void) hcsr04_init(&us, GPIO_NUM_12, GPIO_NUM_13);
    uint32_t usmm = 0;
    (void) hcsr04_read(&us, &usmm);
    ESP_LOGI(TAG, "ds18b20=%d hcsr04=%u", (int) owt, (unsigned) usmm);

    /* saturating_counter compile-gate. */
    saturating_counter sc;
    saturating_counter_init(&sc, 0, 10, 0);
    (void) saturating_counter_inc(&sc);
    (void) saturating_counter_dec(&sc);
    ESP_LOGI(TAG, "satcnt=%d", (int) saturating_counter_get(&sc));

    /* bme280 compile-gate. Touch the pure symbols (parse/compensate on zeroed
       buffers) and the SDK-glue init so the linker pulls in both objects. */
    bme280_calib bme_cal = {0};
    bme280_reading bme_r;
    (void) bme280_parse_calib((const uint8_t[26]){0}, (const uint8_t[7]){0}, &bme_cal);
    (void) bme280_compensate(&bme_cal, 0, 0, 0, &bme_r);
    bme280 bme_dev;
    (void) bme280_init(&bme_dev, BME280_ADDR_PRIMARY);
    ESP_LOGI(TAG, "bme280 temp_mc=%d", (int) bme_r.temp_mc);

    /* L3 ws2812/ir_nec/pir compile-gate. Touch the pure symbols + one glue init
       each so the linker pulls in both objects of every driver. */
    ws2812_rgb ws_px = {0};
    uint8_t ws_buf[3];
    ws2812 ws_dev;
    (void) ws2812_render(&ws_px, 1, 255, ws_buf, sizeof ws_buf);
    (void) ws2812_init(&ws_dev, GPIO_NUM_2, 1, ws_buf, sizeof ws_buf);

    uint16_t ir_us[66] = {0};
    ir_nec_result ir_r;
    ir_nec_rx ir_dev;
    (void) ir_nec_decode(ir_us, 66, &ir_r);
    (void) ir_nec_rx_init(&ir_dev, GPIO_NUM_4);

    pir_gpio pir_dev;
    (void) pir_gpio_init(&pir_dev, GPIO_NUM_5, 1000, true);
    ESP_LOGI(TAG, "l3 ws2812/ir_nec/pir linked");

    /* L3 ssd1306/max7219/rotary compile-gate. Touch the pure symbols + one glue
       init each so the linker pulls in both objects of every driver. */
    uint8_t oled_buf[SSD1306_FB_BYTES(128, 64)];
    ssd1306 oled;
    (void) ssd1306_init(&oled, SSD1306_ADDR, oled_buf, 128, 64);

    uint8_t mx_frame[2];
    max7219 mx;
    max7219_frame(0x01, max7219_digit_segments(8, true), mx_frame);
    (void) max7219_init(&mx, GPIO_NUM_15, 8);

    rotary_gpio knob;
    (void) rotary_gpio_init(&knob, GPIO_NUM_12, GPIO_NUM_13);
    ESP_LOGI(TAG, "l3 ssd1306/max7219/rotary linked");

    /* ssd1306_text compile-gate: pure 5x7 text layer over the oled framebuffer. */
    ssd1306_draw_char(&oled.fb, 0, 0, 'A');
    (void) ssd1306_draw_string(&oled.fb, 0, 8, "hi");

    /* L3 stepper/hx711/rc5 compile-gate. Touch one glue init/op each so the
       linker pulls in both objects (pure + SDK glue) of every driver. */
    stepper_gpio st;
    (void) stepper_gpio_init(&st, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_0, GPIO_NUM_2);
    (void) stepper_gpio_step(&st, true);

    hx711_dev hx;
    (void) hx711_dev_init(&hx, GPIO_NUM_14, GPIO_NUM_12, HX711_GAIN_A128);
    int32_t hx_val = 0;
    (void) hx711_dev_read(&hx, &hx_val, 100);

    rc5_rx rc;
    (void) rc5_rx_init(&rc, GPIO_NUM_13);
    rc5_result rc_r;
    (void) rc5_rx_read(&rc, &rc_r, 100);
    ESP_LOGI(TAG, "l3 stepper/hx711/rc5 linked hx=%d", (int) hx_val);
}
