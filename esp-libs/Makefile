# Host build + test runner for esp-libs Layer-1 pure libs. Reuses common's Unity.
CC     ?= cc
CFLAGS := -std=c11 -Wall -Wextra -g
UNITY  := ../common/third_party/unity
BUILD  := build

.PHONY: test backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse strict clean
test: backoff debounce filter mqtt_topic device_id json_build relay_level dht_parse

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

strict:
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibackoff    -c backoff/backoff.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idebounce   -c debounce/debounce.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifilter     -c filter/filter.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imqtt_topic -c mqtt_topic/mqtt_topic.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idevice_id -c device_id/device_id.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ijson -c json/json_build.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Irelay -c relay/relay_level.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idht -c dht/dht_parse.c -o /dev/null
	@echo "strict: OK"

clean:
	rm -rf $(BUILD)
