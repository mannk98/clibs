# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus spi_bus \
                             ds18b20 servo hcsr04 saturating_counter bme280 ws2812 ir_nec pir \
                             ssd1306 max7219 rotary stepper hx711 rc5 bh1750 sht3x ds3231 \
                             sk6812 ads1115 ina219 tm1637 lcd1602 \
                             ota_version ota_fsm ota_manifest ota_dev
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus spi_bus \
                             ds18b20 servo hcsr04 saturating_counter bme280 ws2812 ir_nec pir \
                             ssd1306 max7219 rotary stepper hx711 rc5 bh1750 sht3x ds3231 \
                             sk6812 ads1115 ina219 tm1637 lcd1602 \
                             ota_version ota_fsm ota_manifest ota_dev
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o button/button.o \
                  dht/dht_parse.o dht/dht.o \
                  pwm_dimmer/dimmer_duty.o pwm_dimmer/pwm_dimmer.o \
                  hysteresis/hysteresis.o crc/crc.o map_range/map_range.o \
                  median/median.o throttle/throttle.o \
                  adc_a0/adc_a0.o i2c_bus/i2c_bus.o spi_bus/spi_bus.o \
                  ds18b20/ds18b20_parse.o ds18b20/ds18b20.o \
                  servo/servo_duty.o servo/servo.o \
                  hcsr04/hcsr04_dist.o hcsr04/hcsr04.o \
                  saturating_counter/saturating_counter.o \
                  bme280/bme280.o bme280/bme280_parse.o \
                  ws2812/ws2812_render.o ws2812/ws2812.o \
                  ir_nec/ir_nec.o ir_nec/ir_nec_rx.o \
                  pir/pir.o pir/pir_gpio.o \
                  ssd1306/ssd1306_fb.o ssd1306/ssd1306.o ssd1306/ssd1306_text.o \
                  ssd1306/ssd1306_gfx.o \
                  max7219/max7219_encode.o max7219/max7219.o \
                  rotary/rotary.o rotary/rotary_gpio.o \
                  stepper/stepper.o stepper/stepper_gpio.o \
                  hx711/hx711.o hx711/hx711_gpio.o \
                  rc5/rc5.o rc5/rc5_rx.o \
                  bh1750/bh1750.o bh1750/bh1750_dev.o \
                  sht3x/sht3x.o sht3x/sht3x_dev.o \
                  ds3231/ds3231.o ds3231/ds3231_dev.o \
                  sk6812/sk6812.o sk6812/sk6812_dev.o \
                  ads1115/ads1115.o ads1115/ads1115_dev.o \
                  ina219/ina219.o ina219/ina219_dev.o \
                  tm1637/tm1637.o tm1637/tm1637_dev.o \
                  lcd1602/lcd1602.o lcd1602/lcd1602_dev.o \
                  ota_version/ota_version.o ota_fsm/ota_fsm.o \
                  ota_manifest/ota_manifest.o ota_dev/ota_dev.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip app_update esp_http_client
