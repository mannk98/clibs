# Host build + test runner for the clibs portable libraries.
CC      ?= cc
CFLAGS  := -std=c11 -Wall -Wextra -g
UNITY   := third_party/unity
BUILD   := build

.PHONY: test str_utils ringbuf dll log bitset mempool fixed crc fsm strict clean
test: str_utils ringbuf dll log bitset mempool fixed crc fsm

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

bitset: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ibitset -o $(BUILD)/test_bitset \
	    bitset/test_bitset.c bitset/bitset.c $(UNITY)/unity.c
	./$(BUILD)/test_bitset

mempool: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imempool -o $(BUILD)/test_mempool \
	    mempool/test_mempool.c mempool/mempool.c $(UNITY)/unity.c
	./$(BUILD)/test_mempool

fixed: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ifixed -o $(BUILD)/test_fixed \
	    fixed/test_fixed.c fixed/fixed.c $(UNITY)/unity.c
	./$(BUILD)/test_fixed

crc: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Icrc -o $(BUILD)/test_crc \
	    crc/test_crc.c crc/crc.c $(UNITY)/unity.c
	./$(BUILD)/test_crc

fsm: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ifsm -o $(BUILD)/test_fsm \
	    fsm/test_fsm.c fsm/fsm.c $(UNITY)/unity.c
	./$(BUILD)/test_fsm

# Strict compile gate: reusable libs must be warning-clean under -Werror.
# log is compiled WITHOUT -Werror (third-party style) but WITH color disabled
# to exercise its non-color branch (catches the tag bug).
strict:
	$(CC) -std=c11 -Wall -Wextra -Werror -Istr_utils -c str_utils/str_utils.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iringbuf   -c ringbuf/ringbuf.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Idll       -c dll/dll.c             -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -DLOG_DISABLE_COLOR -Ilog/src -c log/src/log.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibitset  -c bitset/bitset.c   -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imempool -c mempool/mempool.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifixed   -c fixed/fixed.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Icrc     -c crc/crc.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ifsm     -c fsm/fsm.c         -o /dev/null
	@echo "strict: OK"

clean:
	rm -rf $(BUILD)
