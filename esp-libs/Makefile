# Host build + test runner for esp-libs Layer-1 pure libs. Reuses common's Unity.
CC     ?= cc
CFLAGS := -std=c11 -Wall -Wextra -g
UNITY  := ../common/third_party/unity
BUILD  := build

.PHONY: test backoff debounce filter mqtt_topic strict clean
test: backoff debounce filter mqtt_topic

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

strict:
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibackoff    -c backoff/backoff.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idebounce   -c debounce/debounce.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifilter     -c filter/filter.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imqtt_topic -c mqtt_topic/mqtt_topic.c -o /dev/null
	@echo "strict: OK"

clean:
	rm -rf $(BUILD)
