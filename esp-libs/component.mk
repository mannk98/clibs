# esp-libs as an ESP8266 RTOS SDK component. Library objects only — the host-only
# test_*.c files are NOT compiled into firmware (explicit COMPONENT_OBJS).
COMPONENT_ADD_INCLUDEDIRS := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht
COMPONENT_SRCDIRS         := backoff debounce filter mqtt_topic wifi_sta nvs_kv mqtt_node \
                             device_id json periodic mdns_node relay button dht
COMPONENT_OBJS := backoff/backoff.o debounce/debounce.o filter/filter.o \
                  mqtt_topic/mqtt_topic.o wifi_sta/wifi_sta.o \
                  nvs_kv/nvs_kv.o mqtt_node/mqtt_node.o \
                  device_id/device_id.o device_id/device_id_get.o \
                  json/json_build.o json/json_get.o \
                  periodic/periodic.o mdns_node/mdns_node.o \
                  relay/relay_level.o relay/relay.o button/button.o \
                  dht/dht_parse.o dht/dht.o
COMPONENT_REQUIRES := nvs_flash mqtt mdns json lwip
