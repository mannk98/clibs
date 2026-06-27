# Host build + test runner for the clibs portable libraries.
CC      ?= cc
CFLAGS  := -std=c11 -Wall -Wextra -g
UNITY   := third_party/unity
BUILD   := build

.PHONY: test str_utils ringbuf dll log bitset mempool fixed crc fsm vec pqueue rng hex base64 hashmap stats cobs scheduler event slip tlv softtimer strict sanitize clean
test: str_utils ringbuf dll log bitset mempool fixed crc fsm vec pqueue rng hex base64 hashmap stats cobs scheduler event slip tlv softtimer

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

vec: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ivec -o $(BUILD)/test_vec \
	    vec/test_vec.c vec/vec.c $(UNITY)/unity.c
	./$(BUILD)/test_vec

pqueue: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ipqueue -o $(BUILD)/test_pqueue \
	    pqueue/test_pqueue.c pqueue/pqueue.c $(UNITY)/unity.c
	./$(BUILD)/test_pqueue

rng: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Irng -o $(BUILD)/test_rng \
	    rng/test_rng.c rng/rng.c $(UNITY)/unity.c
	./$(BUILD)/test_rng

hex: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ihex -o $(BUILD)/test_hex \
	    hex/test_hex.c hex/hex.c $(UNITY)/unity.c
	./$(BUILD)/test_hex

base64: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ibase64 -o $(BUILD)/test_base64 \
	    base64/test_base64.c base64/base64.c $(UNITY)/unity.c
	./$(BUILD)/test_base64

hashmap: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ihashmap -o $(BUILD)/test_hashmap \
	    hashmap/test_hashmap.c hashmap/hashmap.c $(UNITY)/unity.c
	./$(BUILD)/test_hashmap

stats: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Istats -o $(BUILD)/test_stats \
	    stats/test_stats.c stats/stats.c $(UNITY)/unity.c
	./$(BUILD)/test_stats

cobs: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Icobs -o $(BUILD)/test_cobs \
	    cobs/test_cobs.c cobs/cobs.c $(UNITY)/unity.c
	./$(BUILD)/test_cobs

scheduler: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ischeduler -o $(BUILD)/test_scheduler \
	    scheduler/test_scheduler.c scheduler/scheduler.c $(UNITY)/unity.c
	./$(BUILD)/test_scheduler

event: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Ievent -o $(BUILD)/test_event \
	    event/test_event.c event/event.c $(UNITY)/unity.c
	./$(BUILD)/test_event

slip: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Islip -o $(BUILD)/test_slip \
	    slip/test_slip.c slip/slip.c $(UNITY)/unity.c
	./$(BUILD)/test_slip

tlv: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Itlv -o $(BUILD)/test_tlv \
	    tlv/test_tlv.c tlv/tlv.c $(UNITY)/unity.c
	./$(BUILD)/test_tlv

softtimer: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Isofttimer -o $(BUILD)/test_softtimer \
	    softtimer/test_softtimer.c softtimer/softtimer.c $(UNITY)/unity.c
	./$(BUILD)/test_softtimer

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
	$(CC) -std=c11 -Wall -Wextra -Werror -Ivec     -c vec/vec.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ipqueue  -c pqueue/pqueue.c   -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Irng     -c rng/rng.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ihex     -c hex/hex.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ibase64  -c base64/base64.c   -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ihashmap   -c hashmap/hashmap.c     -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Istats     -c stats/stats.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Icobs      -c cobs/cobs.c           -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ischeduler -c scheduler/scheduler.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Ievent     -c event/event.c         -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Islip      -c slip/slip.c           -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Itlv       -c tlv/tlv.c             -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Isofttimer -c softtimer/softtimer.c -o /dev/null
	@echo "strict: OK"

# Sanitizer gate: rebuild + run every suite under UBSan + ASan. Catches UB and
# memory bugs that pass `test` and `strict` (e.g. the batch-1 fixed-point
# left-shift-of-negative UB). Target-specific CFLAGS propagate to the `test`
# prerequisite and its per-lib targets (which always recompile).
sanitize: CFLAGS += -fsanitize=undefined,address -fno-sanitize-recover=all
sanitize: test
	@echo "sanitize: OK"

clean:
	rm -rf $(BUILD)
