# Host build + test runner for the clibs portable libraries.
CC      ?= cc
CFLAGS  := -std=c11 -Wall -Wextra -g
UNITY   := third_party/unity
BUILD   := build

.PHONY: test str_utils ringbuf dll log strict clean
test: str_utils ringbuf dll log

$(BUILD):
	mkdir -p $(BUILD)

str_utils: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Istr_utils -o $(BUILD)/test_str_utils \
	    str_utils/test_str_utils.c str_utils/str_utils.c $(UNITY)/unity.c
	./$(BUILD)/test_str_utils

ringbuf: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iringbuf -o $(BUILD)/test_ringbuf \
	    ringbuf/test_ringbuf.c ringbuf/ringbuf.c $(UNITY)/unity.c
	./$(BUILD)/test_ringbuf

dll: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Idll -o $(BUILD)/test_dll \
	    dll/test_dll.c dll/dll.c $(UNITY)/unity.c
	./$(BUILD)/test_dll

log: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ilog/src -o $(BUILD)/test_log \
	    log/test_log.c log/src/log.c $(UNITY)/unity.c
	./$(BUILD)/test_log

# Strict compile gate: reusable libs must be warning-clean under -Werror.
# log is compiled WITHOUT -Werror (third-party style) but WITH color disabled
# to exercise its non-color branch (catches the tag bug).
strict:
	$(CC) -std=c11 -Wall -Wextra -Werror -Istr_utils -c str_utils/str_utils.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iringbuf   -c ringbuf/ringbuf.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idll       -c dll/dll.c             -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -DLOG_DISABLE_COLOR -Ilog/src -c log/src/log.c -o /dev/null
	@echo "strict: OK"

clean:
	rm -rf $(BUILD)
