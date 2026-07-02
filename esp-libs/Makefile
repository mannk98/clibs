# Host build + test runner for esp-libs Layer-1 pure libs. Reuses common's Unity.
CC     ?= cc
CFLAGS := -std=c11 -Wall -Wextra -g
UNITY  := ../common/third_party/unity
BUILD  := build

.PHONY: test backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty hysteresis crc map_range median throttle ds18b20_parse servo_duty hcsr04_dist saturating_counter bme280_parse ws2812_render ir_nec pir ssd1306_fb ssd1306_text ssd1306_gfx max7219_encode rotary stepper rc5 hx711 bh1750 ds3231 sht3x ina219 sk6812 ads1115 tm1637 lcd1602 ota_version ota_fsm ota_manifest strict sanitize clean
test: backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse dimmer_duty hysteresis crc map_range median throttle ds18b20_parse servo_duty hcsr04_dist saturating_counter bme280_parse ws2812_render ir_nec pir ssd1306_fb ssd1306_text ssd1306_gfx max7219_encode rotary stepper rc5 hx711 bh1750 ds3231 sht3x ina219 sk6812 ads1115 tm1637 lcd1602 ota_version ota_fsm ota_manifest

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

pir: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ipir -o $(BUILD)/test_pir \
	    pir/test_pir.c pir/pir.c $(UNITY)/unity.c
	./$(BUILD)/test_pir

ssd1306_fb: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Issd1306 -o $(BUILD)/test_ssd1306_fb \
	    ssd1306/test_ssd1306_fb.c ssd1306/ssd1306_fb.c $(UNITY)/unity.c
	./$(BUILD)/test_ssd1306_fb

ssd1306_text: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Issd1306 -o $(BUILD)/test_ssd1306_text \
	    ssd1306/test_ssd1306_text.c ssd1306/ssd1306_text.c ssd1306/ssd1306_fb.c $(UNITY)/unity.c
	./$(BUILD)/test_ssd1306_text

ssd1306_gfx: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Issd1306 -o $(BUILD)/test_ssd1306_gfx \
	    ssd1306/test_ssd1306_gfx.c ssd1306/ssd1306_gfx.c ssd1306/ssd1306_fb.c $(UNITY)/unity.c
	./$(BUILD)/test_ssd1306_gfx

max7219_encode: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imax7219 -o $(BUILD)/test_max7219_encode \
	    max7219/test_max7219_encode.c max7219/max7219_encode.c $(UNITY)/unity.c
	./$(BUILD)/test_max7219_encode

rotary: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Irotary -o $(BUILD)/test_rotary \
	    rotary/test_rotary.c rotary/rotary.c $(UNITY)/unity.c
	./$(BUILD)/test_rotary

stepper: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Istepper -o $(BUILD)/test_stepper \
	    stepper/test_stepper.c stepper/stepper.c $(UNITY)/unity.c
	./$(BUILD)/test_stepper

rc5: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Irc5 -o $(BUILD)/test_rc5 \
	    rc5/test_rc5.c rc5/rc5.c $(UNITY)/unity.c
	./$(BUILD)/test_rc5

hx711: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ihx711 -o $(BUILD)/test_hx711 \
	    hx711/test_hx711.c hx711/hx711.c $(UNITY)/unity.c
	./$(BUILD)/test_hx711

bh1750: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ibh1750 -o $(BUILD)/test_bh1750 \
	    bh1750/test_bh1750.c bh1750/bh1750.c $(UNITY)/unity.c
	./$(BUILD)/test_bh1750

ds3231: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ids3231 -o $(BUILD)/test_ds3231 \
	    ds3231/test_ds3231.c ds3231/ds3231.c $(UNITY)/unity.c
	./$(BUILD)/test_ds3231

sht3x: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Isht3x -o $(BUILD)/test_sht3x \
	    sht3x/test_sht3x.c sht3x/sht3x.c $(UNITY)/unity.c
	./$(BUILD)/test_sht3x

ads1115: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iads1115 -o $(BUILD)/test_ads1115 \
	    ads1115/test_ads1115.c ads1115/ads1115.c $(UNITY)/unity.c
	./$(BUILD)/test_ads1115

ina219: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iina219 -o $(BUILD)/test_ina219 \
	    ina219/test_ina219.c ina219/ina219.c $(UNITY)/unity.c
	./$(BUILD)/test_ina219

sk6812: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Isk6812 -o $(BUILD)/test_sk6812 \
	    sk6812/test_sk6812.c sk6812/sk6812.c $(UNITY)/unity.c
	./$(BUILD)/test_sk6812

tm1637: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Itm1637 -o $(BUILD)/test_tm1637 \
	    tm1637/test_tm1637.c tm1637/tm1637.c $(UNITY)/unity.c
	./$(BUILD)/test_tm1637

lcd1602: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ilcd1602 -o $(BUILD)/test_lcd1602 \
	    lcd1602/test_lcd1602.c lcd1602/lcd1602.c $(UNITY)/unity.c
	./$(BUILD)/test_lcd1602

ota_version: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iota_version -o $(BUILD)/test_ota_version \
	    ota_version/test_ota_version.c ota_version/ota_version.c $(UNITY)/unity.c
	./$(BUILD)/test_ota_version

ota_fsm: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iota_fsm -o $(BUILD)/test_ota_fsm \
	    ota_fsm/test_ota_fsm.c ota_fsm/ota_fsm.c $(UNITY)/unity.c
	./$(BUILD)/test_ota_fsm

ota_manifest: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iota_manifest -Iota_version -o $(BUILD)/test_ota_manifest \
	    ota_manifest/test_ota_manifest.c ota_manifest/ota_manifest.c ota_version/ota_version.c $(UNITY)/unity.c
	./$(BUILD)/test_ota_manifest

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
	$(CC) -std=c11 -Wall -Wextra -Werror -Ipir    -c pir/pir.c               -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_fb.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_text.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Issd1306 -c ssd1306/ssd1306_gfx.c      -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imax7219 -c max7219/max7219_encode.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Irotary  -c rotary/rotary.c            -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Istepper -c stepper/stepper.c          -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Irc5     -c rc5/rc5.c                 -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ihx711  -c hx711/hx711.c            -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibh1750 -c bh1750/bh1750.c          -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ids3231 -c ds3231/ds3231.c          -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Isht3x -c sht3x/sht3x.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iina219 -c ina219/ina219.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Isk6812 -c sk6812/sk6812.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iads1115 -c ads1115/ads1115.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Itm1637 -c tm1637/tm1637.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ilcd1602 -c lcd1602/lcd1602.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iota_version -c ota_version/ota_version.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iota_fsm -c ota_fsm/ota_fsm.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iota_manifest -Iota_version -c ota_manifest/ota_manifest.c -o /dev/null
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
