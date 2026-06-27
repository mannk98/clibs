# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus spi_bus \
                             ds18b20 servo hcsr04 saturating_counter bme280 ws2812 ir_nec pir
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht pwm_dimmer \
                             hysteresis crc map_range median throttle adc_a0 i2c_bus spi_bus \
                             ds18b20 servo hcsr04 saturating_counter bme280 ws2812 ir_nec pir
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
                  pir/pir.o pir/pir_gpio.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
