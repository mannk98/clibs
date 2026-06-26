# Host build + test runner + AVR cross-compile gate for avr-libs.
CC       ?= cc
AVR_CC   ?= /opt/homebrew/opt/avr-gcc@9/bin/avr-gcc
CFLAGS   := -std=c11 -Wall -Wextra -g
AVRFLAGS := -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall -Wextra
UNITY    := third_party/unity
BUILD    := build

.PHONY: test pid goot80 t33 ringbuf max6675 encoder wallclock led7seg strict avr clean
test: pid goot80 t33 ringbuf max6675 encoder wallclock led7seg

$(BUILD):
	mkdir -p $(BUILD)

pid: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -ImyPid -o $(BUILD)/test_pid \
	    myPid/test_pid.c myPid/PID.c $(UNITY)/unity.c
	./$(BUILD)/test_pid

goot80: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Igoot80_typeK -o $(BUILD)/test_goot80 \
	    goot80_typeK/test_goot80.c goot80_typeK/typeK.c $(UNITY)/unity.c
	./$(BUILD)/test_goot80

t33: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -It33Soldering -o $(BUILD)/test_t33 \
	    t33Soldering/test_t33.c t33Soldering/T33Soldering.c $(UNITY)/unity.c
	./$(BUILD)/test_t33

ringbuf: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -IringBuffer -o $(BUILD)/test_ringbuf \
	    ringBuffer/test_ringbuf.c ringBuffer/ringbuff.c $(UNITY)/unity.c
	./$(BUILD)/test_ringbuf

max6675: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Imax6675 -o $(BUILD)/test_max6675 \
	    max6675/test_max6675_conv.c max6675/max6675_conv.c $(UNITY)/unity.c
	./$(BUILD)/test_max6675

encoder: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iencoder -o $(BUILD)/test_encoder \
	    encoder/test_encoder_decode.c encoder/encoder_decode.c $(UNITY)/unity.c
	./$(BUILD)/test_encoder

wallclock: | $(BUILD)
	$(CC) $(CFLAGS) -DF_CPU=16000000UL -I$(UNITY) -IwallClock -o $(BUILD)/test_wallclock \
	    wallClock/test_wallclock_tick.c wallClock/wallclock_tick.c $(UNITY)/unity.c
	./$(BUILD)/test_wallclock

led7seg: | $(BUILD)
	$(CC) $(CFLAGS) -I$(UNITY) -Iled7seg_font -o $(BUILD)/test_led7seg \
	    led7seg_font/test_led7seg_font.c led7seg_font/led7seg_font.c $(UNITY)/unity.c
	./$(BUILD)/test_led7seg

strict:
	$(CC) -std=c11 -Wall -Wextra -Werror -ImyPid        -c myPid/PID.c                -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Igoot80_typeK -c goot80_typeK/typeK.c        -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -It33Soldering -c t33Soldering/T33Soldering.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -IringBuffer   -c ringBuffer/ringbuff.c       -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Imax6675      -c max6675/max6675_conv.c      -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iencoder      -c encoder/encoder_decode.c    -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -DF_CPU=16000000UL -IwallClock -c wallClock/wallclock_tick.c -o /dev/null
	$(CC) -std=c11 -Wall -Wextra -Werror -Iled7seg_font -c led7seg_font/led7seg_font.c -o /dev/null
	@echo "strict: OK"

avr:
	$(AVR_CC) $(AVRFLAGS) -c myPid/PID.c                 -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c goot80_typeK/typeK.c        -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c t33Soldering/T33Soldering.c -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c ringBuffer/ringbuff.c       -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c max6675/max6675_conv.c      -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c encoder/encoder_decode.c    -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c wallClock/wallclock_tick.c  -o /dev/null
	$(AVR_CC) $(AVRFLAGS) -c led7seg_font/led7seg_font.c -o /dev/null
	@echo "avr: OK"

clean:
	rm -rf $(BUILD)
