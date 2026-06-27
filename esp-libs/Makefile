# Host build + test runner for esp-libs Layer-1 pure libs. Reuses common's Unity.
CC     ?= cc
CFLAGS := -std=c11 -Wall -Wextra -g
UNITY  := ../common/third_party/unity
BUILD  := build

.PHONY: test backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty hysteresis crc map_range median throttle ds18b20_parse servo_duty hcsr04_dist saturating_counter bme280_parse ws2812_render ir_nec strict sanitize clean
test: backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty hysteresis crc map_range median throttle ds18b20_parse servo_duty hcsr04_dist saturating_counter bme280_parse ws2812_render ir_nec

$(BUILD):
	mkdir -p $(BUILD)

backoff: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ibackoff -o $(BUILD)/test_backoff \
	    backoff/test_backoff.c backoff/backoff.c $(UNITY)/unity.c
	./$(BUILD)/test_backoff

debounce: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Idebounce -o $(BUILD)/test_debounce \
	    debounce/test_debounce.c debounce/debounce.c $(UNITY)/unity.c
	./$(BUILD)/test_debounce

filter: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ifilter -o $(BUILD)/test_filter \
	    filter/test_filter.c filter/filter.c $(UNITY)/unity.c
	./$(BUILD)/test_filter

mqtt_topic: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imqtt_topic -o $(BUILD)/test_mqtt_topic \
	    mqtt_topic/test_mqtt_topic.c mqtt_topic/mqtt_topic.c $(UNITY)/unity.c
	./$(BUILD)/test_mqtt_topic

device_id: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Idevice_id -o $(BUILD)/test_device_id \
	    device_id/test_device_id.c device_id/device_id.c $(UNITY)/unity.c
	./$(BUILD)/test_device_id

json_build: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ijson -o $(BUILD)/test_json_build \
	    json/test_json_build.c json/json_build.c $(UNITY)/unity.c
	./$(BUILD)/test_json_build

relay_level: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Irelay -o $(BUILD)/test_relay_level \
	    relay/test_relay_level.c relay/relay_level.c $(UNITY)/unity.c
	./$(BUILD)/test_relay_level

dht_parse: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Idht -o $(BUILD)/test_dht_parse \
	    dht/test_dht_parse.c dht/dht_parse.c $(UNITY)/unity.c
	./$(BUILD)/test_dht_parse

dimmer_duty: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ipwm_dimmer -o $(BUILD)/test_dimmer_duty \
	    pwm_dimmer/test_dimmer_duty.c pwm_dimmer/dimmer_duty.c $(UNITY)/unity.c
	./$(BUILD)/test_dimmer_duty

hysteresis: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ihysteresis -o $(BUILD)/test_hysteresis \
	    hysteresis/test_hysteresis.c hysteresis/hysteresis.c $(UNITY)/unity.c
	./$(BUILD)/test_hysteresis

crc: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Icrc -o $(BUILD)/test_crc \
	    crc/test_crc.c crc/crc.c $(UNITY)/unity.c
	./$(BUILD)/test_crc

map_range: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imap_range -o $(BUILD)/test_map_range \
	    map_range/test_map_range.c map_range/map_range.c $(UNITY)/unity.c
	./$(BUILD)/test_map_range

median: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imedian -o $(BUILD)/test_median \
	    median/test_median.c median/median.c $(UNITY)/unity.c
	./$(BUILD)/test_median

throttle: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ithrottle -o $(BUILD)/test_throttle \
	    throttle/test_throttle.c throttle/throttle.c $(UNITY)/unity.c
	./$(BUILD)/test_throttle

ds18b20_parse: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ids18b20 -Icrc -o $(BUILD)/test_ds18b20_parse \
	    ds18b20/test_ds18b20_parse.c ds18b20/ds18b20_parse.c crc/crc.c $(UNITY)/unity.c
	./$(BUILD)/test_ds18b20_parse

servo_duty: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iservo -o $(BUILD)/test_servo_duty \
	    servo/test_servo_duty.c servo/servo_duty.c $(UNITY)/unity.c
	./$(BUILD)/test_servo_duty

hcsr04_dist: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ihcsr04 -o $(BUILD)/test_hcsr04_dist \
	    hcsr04/test_hcsr04_dist.c hcsr04/hcsr04_dist.c $(UNITY)/unity.c
	./$(BUILD)/test_hcsr04_dist

saturating_counter: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Isaturating_counter -o $(BUILD)/test_saturating_counter \
	    saturating_counter/test_saturating_counter.c saturating_counter/saturating_counter.c $(UNITY)/unity.c
	./$(BUILD)/test_saturating_counter

bme280_parse: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ibme280 -o $(BUILD)/test_bme280_parse \
	    bme280/test_bme280_parse.c bme280/bme280_parse.c $(UNITY)/unity.c
	./$(BUILD)/test_bme280_parse

ws2812_render: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iws2812 -o $(BUILD)/test_ws2812_render \
	    ws2812/test_ws2812_render.c ws2812/ws2812_render.c $(UNITY)/unity.c
	./$(BUILD)/test_ws2812_render

ir_nec: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iir_nec -o $(BUILD)/test_ir_nec \
	    ir_nec/test_ir_nec.c ir_nec/ir_nec.c $(UNITY)/unity.c
	./$(BUILD)/test_ir_nec

strict:
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibackoff    -c backoff/backoff.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idebounce   -c debounce/debounce.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifilter     -c filter/filter.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imqtt_topic -c mqtt_topic/mqtt_topic.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idevice_id -c device_id/device_id.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ijson -c json/json_build.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Irelay -c relay/relay_level.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idht -c dht/dht_parse.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ipwm_dimmer -c pwm_dimmer/dimmer_duty.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ihysteresis -c hysteresis/hysteresis.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Icrc        -c crc/crc.c               -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imap_range  -c map_range/map_range.c   -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imedian     -c median/median.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ithrottle   -c throttle/throttle.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ids18b20 -Icrc -c ds18b20/ds18b20_parse.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iservo       -c servo/servo_duty.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ihcsr04      -c hcsr04/hcsr04_dist.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Isaturating_counter -c saturating_counter/saturating_counter.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibme280 -c bme280/bme280_parse.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iws2812 -c ws2812/ws2812_render.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iir_nec -c ir_nec/ir_nec.c -o /dev/null
	@echo "strict: OK"

# Sanitizer gate: rebuild + run every host suite under UBSan + ASan. Catches UB
# (e.g. left-shift of a negative value in the bme280 Bosch fixed-point math) that
# `test` and `strict` miss. Carried over from common/Makefile. Target-specific
# CFLAGS propagate to the `test` prerequisite and its per-lib targets.
sanitize: CFLAGS += -fsanitize=undefined,address -fno-sanitize-recover=all
sanitize: test
	@echo "sanitize: OK"

clean:
	rm -rf $(BUILD)
