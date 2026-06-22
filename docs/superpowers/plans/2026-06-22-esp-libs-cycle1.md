# esp-libs Cycle 1 (Layer-1 Pure Building Blocks) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create `clibs/esp-libs/` with four pure-logic, host-Unity-tested building blocks (`backoff`, `debounce`, `filter`/EMA, `mqtt_topic`) for future ESP8266 smart-home nodes.

**Architecture:** Plain portable C (no SDK/hardware) — each lib is a small self-contained folder with a Unity suite, built by an `esp-libs/Makefile` that reuses `../common/third_party/unity`. Compiles on host now, on ESP8266 (newlib) unchanged later.

**Tech Stack:** C11/C99, Unity (reused from `common/third_party/unity`), GNU make, host `cc`.

**Spec:** `clibs/docs/superpowers/specs/2026-06-22-esp-libs-cycle1-pure-logic-design.md`

## Global Constraints
- Code lives in **`clibs/esp-libs/`**, committed **directly in the `clibs` repo** (NOT a submodule). Branch created by the controller; implementers must NOT switch branches. All `git` runs in `/Users/man.nk/git/clibs`.
- **Freestanding**: `<stdint.h>`/`<stdbool.h>`/`<stddef.h>` only (+`<string.h>` in `mqtt_topic`). Static / **no malloc**. Transparent struct + `self`-methods. `CLIBS_*_H` guards. NULL-arg guards. Doc comment per public function.
- Host-only this cycle: `make test` + `make strict` (`-Werror`). No ESP cross-compile gate (no toolchain yet).
- Reuse `../common/third_party/unity` — do NOT vendor a second Unity copy.
- Append to every commit message:
  ```
  Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>
  ```

## File Structure

| File | Responsibility |
|------|----------------|
| `esp-libs/Makefile`, `esp-libs/.gitignore`, `esp-libs/README.md` | scaffold (host tests + strict; ignore `build/`) |
| `esp-libs/backoff/{backoff.h,backoff.c,test_backoff.c}` | exponential reconnect backoff |
| `esp-libs/debounce/{debounce.h,debounce.c,test_debounce.c}` | input debounce |
| `esp-libs/filter/{filter.h,filter.c,test_filter.c}` | integer EMA |
| `esp-libs/mqtt_topic/{mqtt_topic.h,mqtt_topic.c,test_mqtt_topic.c}` | topic join + wildcard match |

---

## Task 1: Scaffold + `backoff`

**Files:**
- Create: `esp-libs/Makefile`, `esp-libs/.gitignore`, `esp-libs/backoff/backoff.h`, `esp-libs/backoff/backoff.c`, `esp-libs/backoff/test_backoff.c`

**Interfaces:**
- Produces: `void backoff_init(backoff*, uint32_t base_ms, uint32_t max_ms)`, `uint32_t backoff_next(backoff*)`, `void backoff_reset(backoff*)`.

- [ ] **Step 1: Create `esp-libs/.gitignore`**
```
build/
```

- [ ] **Step 2: Create `esp-libs/Makefile`** (recipe lines MUST be real TABs):
```makefile
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
```
(Note: `make test`/`strict` won't fully succeed until later tasks add the other libs — that's expected; this task only runs `make backoff`.)

- [ ] **Step 3: Write the failing test** — Create `esp-libs/backoff/test_backoff.c`:
```c
#include "unity.h"
#include "backoff.h"

void setUp(void) {}
void tearDown(void) {}

static void test_sequence_doubles_and_caps(void) {
    backoff b;
    backoff_init(&b, 100, 1000);
    TEST_ASSERT_EQUAL_UINT32(100,  backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(200,  backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(400,  backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(800,  backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(1000, backoff_next(&b)); /* min(1600,1000) */
    TEST_ASSERT_EQUAL_UINT32(1000, backoff_next(&b)); /* stays capped */
}
static void test_reset(void) {
    backoff b;
    backoff_init(&b, 100, 1000);
    backoff_next(&b); backoff_next(&b);
    backoff_reset(&b);
    TEST_ASSERT_EQUAL_UINT32(100, backoff_next(&b));
}
static void test_max_below_base_clamps(void) {
    backoff b;
    backoff_init(&b, 500, 100); /* max < base -> max becomes 500 */
    TEST_ASSERT_EQUAL_UINT32(500, backoff_next(&b));
    TEST_ASSERT_EQUAL_UINT32(500, backoff_next(&b));
}
static void test_null_guard(void) {
    TEST_ASSERT_EQUAL_UINT32(0, backoff_next(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sequence_doubles_and_caps);
    RUN_TEST(test_reset);
    RUN_TEST(test_max_below_base_clamps);
    RUN_TEST(test_null_guard);
    return UNITY_END();
}
```

- [ ] **Step 4: Run, expect FAIL** — `cd /Users/man.nk/git/clibs/esp-libs && make backoff`
Expected: compile/link error — `backoff.h` / functions don't exist.

- [ ] **Step 5: Create `esp-libs/backoff/backoff.h`**
```c
#ifndef CLIBS_BACKOFF_H
#define CLIBS_BACKOFF_H

#include <stdint.h>

/* Exponential reconnect backoff (e.g. for WiFi/MQTT). Not thread-safe. */
typedef struct {
    uint32_t base_ms;
    uint32_t max_ms;
    uint32_t cur_ms;
} backoff;

/* Initialize: cur = base; if max < base, max is raised to base. */
void backoff_init(backoff *self, uint32_t base_ms, uint32_t max_ms);
/* Return the current delay, then double it toward max_ms (overflow-safe cap). */
uint32_t backoff_next(backoff *self);
/* Reset cur back to base (call on a successful connect). */
void backoff_reset(backoff *self);

#endif /* CLIBS_BACKOFF_H */
```

- [ ] **Step 6: Create `esp-libs/backoff/backoff.c`**
```c
#include "backoff.h"
#include <stddef.h>

void backoff_init(backoff *self, uint32_t base_ms, uint32_t max_ms) {
    if (self == NULL) return;
    if (max_ms < base_ms) max_ms = base_ms;
    self->base_ms = base_ms;
    self->max_ms  = max_ms;
    self->cur_ms  = base_ms;
}

uint32_t backoff_next(backoff *self) {
    if (self == NULL) return 0;
    uint32_t d = self->cur_ms;
    self->cur_ms = (self->cur_ms <= self->max_ms / 2) ? (self->cur_ms * 2) : self->max_ms;
    return d;
}

void backoff_reset(backoff *self) {
    if (self == NULL) return;
    self->cur_ms = self->base_ms;
}
```

- [ ] **Step 7: Run, expect PASS** — `cd /Users/man.nk/git/clibs/esp-libs && make backoff`
Expected: `4 Tests 0 Failures 0 Ignored` / `OK`. Paste output.

- [ ] **Step 8: Confirm strict** — `cd /Users/man.nk/git/clibs/esp-libs && cc -std=c11 -Wall -Wextra -Werror -Ibackoff -c backoff/backoff.c -o /dev/null && echo STRICT_OK`

- [ ] **Step 9: Commit**
```bash
cd /Users/man.nk/git/clibs
git add esp-libs/.gitignore esp-libs/Makefile esp-libs/backoff
git commit -m "feat(esp-libs): scaffold + backoff (exponential reconnect backoff) + tests"
```

---

## Task 2: `debounce`

**Files:**
- Create: `esp-libs/debounce/debounce.h`, `esp-libs/debounce/debounce.c`, `esp-libs/debounce/test_debounce.c`

**Interfaces:**
- Produces: `debounce_edge` enum `{DEBOUNCE_NONE, DEBOUNCE_RISING, DEBOUNCE_FALLING}`; `void debounce_init(debounce*, uint8_t threshold, uint8_t initial)`; `debounce_edge debounce_update(debounce*, uint8_t raw)`; `uint8_t debounce_level(const debounce*)`.

- [ ] **Step 1: Write the failing test** — Create `esp-libs/debounce/test_debounce.c`:
```c
#include "unity.h"
#include "debounce.h"

void setUp(void) {}
void tearDown(void) {}

static void test_rising_after_threshold(void) {
    debounce d;
    debounce_init(&d, 3, 0);
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE,   debounce_update(&d, 1));
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE,   debounce_update(&d, 1));
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_RISING, debounce_update(&d, 1));
    TEST_ASSERT_EQUAL_UINT8(1, debounce_level(&d));
}
static void test_falling_after_threshold(void) {
    debounce d;
    debounce_init(&d, 2, 1);
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE,    debounce_update(&d, 0));
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_FALLING, debounce_update(&d, 0));
    TEST_ASSERT_EQUAL_UINT8(0, debounce_level(&d));
}
static void test_bounce_ignored(void) {
    debounce d;
    debounce_init(&d, 3, 0);
    debounce_update(&d, 1);                                       /* count 1 */
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE, debounce_update(&d, 0)); /* back to stable */
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_NONE, debounce_update(&d, 1)); /* count 1 again */
    TEST_ASSERT_EQUAL_UINT8(0, debounce_level(&d));               /* never committed */
}
static void test_threshold_zero_becomes_one(void) {
    debounce d;
    debounce_init(&d, 0, 0);
    TEST_ASSERT_EQUAL_INT(DEBOUNCE_RISING, debounce_update(&d, 1)); /* threshold clamped to 1 */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rising_after_threshold);
    RUN_TEST(test_falling_after_threshold);
    RUN_TEST(test_bounce_ignored);
    RUN_TEST(test_threshold_zero_becomes_one);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, expect FAIL** — `cd /Users/man.nk/git/clibs/esp-libs && make debounce`
Expected: compile/link error — `debounce.h` / functions don't exist.

- [ ] **Step 3: Create `esp-libs/debounce/debounce.h`**
```c
#ifndef CLIBS_DEBOUNCE_H
#define CLIBS_DEBOUNCE_H

#include <stdint.h>
#include <stdbool.h>

/* Digital input debounce. Caller samples the raw level at a fixed interval and
 * feeds it to debounce_update; a change is committed only after `threshold`
 * consecutive samples of the new level. Not thread-safe. */
typedef enum { DEBOUNCE_NONE, DEBOUNCE_RISING, DEBOUNCE_FALLING } debounce_edge;

typedef struct {
    uint8_t stable;     /* committed debounced level (0/1) */
    uint8_t candidate;  /* level currently being counted */
    uint8_t count;      /* consecutive samples of candidate */
    uint8_t threshold;  /* samples needed to commit a change (>=1) */
} debounce;

/* threshold 0 is clamped to 1; stable starts at (initial ? 1 : 0). */
void          debounce_init(debounce *self, uint8_t threshold, uint8_t initial);
/* Feed one sampled level; returns the edge when a change is committed, else NONE. */
debounce_edge debounce_update(debounce *self, uint8_t raw);
/* Current committed level (0/1). */
uint8_t       debounce_level(const debounce *self);

#endif /* CLIBS_DEBOUNCE_H */
```

- [ ] **Step 4: Create `esp-libs/debounce/debounce.c`**
```c
#include "debounce.h"
#include <stddef.h>

void debounce_init(debounce *self, uint8_t threshold, uint8_t initial) {
    if (self == NULL) return;
    self->threshold = (threshold == 0) ? 1 : threshold;
    self->stable    = initial ? 1 : 0;
    self->candidate = self->stable;
    self->count     = 0;
}

debounce_edge debounce_update(debounce *self, uint8_t raw) {
    if (self == NULL) return DEBOUNCE_NONE;
    raw = raw ? 1 : 0;

    if (raw == self->stable) {       /* no pending change */
        self->count = 0;
        return DEBOUNCE_NONE;
    }
    if (raw == self->candidate) {
        if (self->count < 0xFF) self->count++;
    } else {
        self->candidate = raw;
        self->count = 1;
    }
    if (self->count >= self->threshold) {
        self->stable = raw;
        self->count = 0;
        return raw ? DEBOUNCE_RISING : DEBOUNCE_FALLING;
    }
    return DEBOUNCE_NONE;
}

uint8_t debounce_level(const debounce *self) {
    return self ? self->stable : 0;
}
```

- [ ] **Step 5: Run, expect PASS** — `cd /Users/man.nk/git/clibs/esp-libs && make debounce`
Expected: `4 Tests 0 Failures 0 Ignored` / `OK`. Paste output.

- [ ] **Step 6: Confirm strict** — `cd /Users/man.nk/git/clibs/esp-libs && cc -std=c11 -Wall -Wextra -Werror -Idebounce -c debounce/debounce.c -o /dev/null && echo STRICT_OK`

- [ ] **Step 7: Commit**
```bash
cd /Users/man.nk/git/clibs
git add esp-libs/debounce
git commit -m "feat(esp-libs/debounce): input debounce with edge detection + tests"
```

---

## Task 3: `filter` (integer EMA)

**Files:**
- Create: `esp-libs/filter/filter.h`, `esp-libs/filter/filter.c`, `esp-libs/filter/test_filter.c`

**Interfaces:**
- Produces: `ema` struct `{int32_t acc; uint8_t shift;}`; `void ema_init(ema*, uint8_t shift, int32_t initial)`; `int32_t ema_update(ema*, int32_t sample)`.

- [ ] **Step 1: Write the failing test** — Create `esp-libs/filter/test_filter.c`:
```c
#include "unity.h"
#include "filter.h"

void setUp(void) {}
void tearDown(void) {}

static void test_first_steps(void) {
    ema f;
    ema_init(&f, 2, 0);                                  /* alpha = 1/4 */
    TEST_ASSERT_EQUAL_INT32(25, ema_update(&f, 100));    /* acc=100 -> 100>>2=25 */
    TEST_ASSERT_EQUAL_INT32(43, ema_update(&f, 100));    /* acc=175 -> 175>>2=43 */
}
static void test_converges_to_constant(void) {
    ema f;
    ema_init(&f, 3, 0);
    int32_t v = 0;
    for (int i = 0; i < 100; i++) v = ema_update(&f, 500);
    TEST_ASSERT_INT32_WITHIN(2, 500, v);
}
static void test_steady_state_stable(void) {
    ema f;
    ema_init(&f, 4, 500);                                /* start at the steady value */
    TEST_ASSERT_EQUAL_INT32(500, ema_update(&f, 500));
    TEST_ASSERT_EQUAL_INT32(500, ema_update(&f, 500));
}
static void test_null_guard(void) {
    TEST_ASSERT_EQUAL_INT32(0, ema_update(NULL, 100));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_first_steps);
    RUN_TEST(test_converges_to_constant);
    RUN_TEST(test_steady_state_stable);
    RUN_TEST(test_null_guard);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, expect FAIL** — `cd /Users/man.nk/git/clibs/esp-libs && make filter`
Expected: compile/link error — `filter.h` / functions don't exist.

- [ ] **Step 3: Create `esp-libs/filter/filter.h`**
```c
#ifndef CLIBS_FILTER_H
#define CLIBS_FILTER_H

#include <stdint.h>

/* Integer exponential moving average. alpha = 1/2^shift. `acc` is the value in
 * fixed-point (value << shift). Keep |value| << shift within int32 range
 * (fine for ADC 0..1023 with shift <= 8). Not thread-safe. */
typedef struct {
    int32_t acc;
    uint8_t shift;
} ema;

void    ema_init(ema *self, uint8_t shift, int32_t initial);
int32_t ema_update(ema *self, int32_t sample); /* returns the new smoothed value */

#endif /* CLIBS_FILTER_H */
```

- [ ] **Step 4: Create `esp-libs/filter/filter.c`**
```c
#include "filter.h"
#include <stddef.h>

void ema_init(ema *self, uint8_t shift, int32_t initial) {
    if (self == NULL) return;
    self->shift = shift;
    self->acc   = (int32_t)((uint32_t)initial << shift); /* well-defined for the documented range */
}

int32_t ema_update(ema *self, int32_t sample) {
    if (self == NULL) return 0;
    self->acc += sample - (self->acc >> self->shift);
    return self->acc >> self->shift;
}
```

- [ ] **Step 5: Run, expect PASS** — `cd /Users/man.nk/git/clibs/esp-libs && make filter`
Expected: `4 Tests 0 Failures 0 Ignored` / `OK`. Paste output.

- [ ] **Step 6: Confirm strict** — `cd /Users/man.nk/git/clibs/esp-libs && cc -std=c11 -Wall -Wextra -Werror -Ifilter -c filter/filter.c -o /dev/null && echo STRICT_OK`

- [ ] **Step 7: Commit**
```bash
cd /Users/man.nk/git/clibs
git add esp-libs/filter
git commit -m "feat(esp-libs/filter): integer EMA smoothing + tests"
```

---

## Task 4: `mqtt_topic`

**Files:**
- Create: `esp-libs/mqtt_topic/mqtt_topic.h`, `esp-libs/mqtt_topic/mqtt_topic.c`, `esp-libs/mqtt_topic/test_mqtt_topic.c`

**Interfaces:**
- Produces: `int mqtt_topic_join(char *buf, size_t n, const char *const *parts, size_t count)`; `bool mqtt_topic_match(const char *filter, const char *topic)`.

- [ ] **Step 1: Write the failing test** — Create `esp-libs/mqtt_topic/test_mqtt_topic.c`:
```c
#include "unity.h"
#include "mqtt_topic.h"

void setUp(void) {}
void tearDown(void) {}

static void test_join_basic(void) {
    const char *parts[] = { "home", "kitchen", "temp" };
    char buf[32];
    TEST_ASSERT_EQUAL_INT(17, mqtt_topic_join(buf, sizeof buf, parts, 3));
    TEST_ASSERT_EQUAL_STRING("home/kitchen/temp", buf);
}
static void test_join_empty(void) {
    char buf[8];
    TEST_ASSERT_EQUAL_INT(0, mqtt_topic_join(buf, sizeof buf, NULL, 0));
    TEST_ASSERT_EQUAL_STRING("", buf);
}
static void test_join_truncation(void) {
    const char *parts[] = { "home", "kitchen", "temp" };
    char buf[8];
    TEST_ASSERT_EQUAL_INT(-1, mqtt_topic_join(buf, sizeof buf, parts, 3));
    TEST_ASSERT_EQUAL_STRING("", buf);
}
static void test_match_plus(void) {
    TEST_ASSERT_TRUE (mqtt_topic_match("home/+/temp", "home/kitchen/temp"));
    TEST_ASSERT_FALSE(mqtt_topic_match("home/+/temp", "home/a/b/temp"));
    TEST_ASSERT_TRUE (mqtt_topic_match("+", "a"));
    TEST_ASSERT_FALSE(mqtt_topic_match("+", "a/b"));
}
static void test_match_hash(void) {
    TEST_ASSERT_TRUE(mqtt_topic_match("home/#", "home/a/b"));
    TEST_ASSERT_TRUE(mqtt_topic_match("home/#", "home"));   /* parent match */
    TEST_ASSERT_TRUE(mqtt_topic_match("#", "anything/at/all"));
}
static void test_match_exact(void) {
    TEST_ASSERT_TRUE (mqtt_topic_match("a/b", "a/b"));
    TEST_ASSERT_FALSE(mqtt_topic_match("a/b", "a/b/c"));
    TEST_ASSERT_FALSE(mqtt_topic_match("a/b", "a"));
    TEST_ASSERT_FALSE(mqtt_topic_match("a/b/c", "a/b"));
}
static void test_null_guard(void) {
    TEST_ASSERT_FALSE(mqtt_topic_match(NULL, "a"));
    TEST_ASSERT_EQUAL_INT(-1, mqtt_topic_join(NULL, 10, NULL, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_join_basic);
    RUN_TEST(test_join_empty);
    RUN_TEST(test_join_truncation);
    RUN_TEST(test_match_plus);
    RUN_TEST(test_match_hash);
    RUN_TEST(test_match_exact);
    RUN_TEST(test_null_guard);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, expect FAIL** — `cd /Users/man.nk/git/clibs/esp-libs && make mqtt_topic`
Expected: compile/link error — `mqtt_topic.h` / functions don't exist.

- [ ] **Step 3: Create `esp-libs/mqtt_topic/mqtt_topic.h`**
```c
#ifndef CLIBS_MQTT_TOPIC_H
#define CLIBS_MQTT_TOPIC_H

#include <stddef.h>
#include <stdbool.h>

/* Join `count` parts with '/' into buf[n] (NUL-terminated). Returns the string
 * length (excl. NUL), or -1 on truncation / NULL buf / NULL part. count 0 -> "". */
int mqtt_topic_join(char *buf, size_t n, const char *const *parts, size_t count);

/* MQTT topic-filter match: '+' matches exactly one level, '#' matches the
 * remaining levels (zero or more; must be the final level, e.g. "a/#" also
 * matches "a"). Returns false on NULL args. */
bool mqtt_topic_match(const char *filter, const char *topic);

#endif /* CLIBS_MQTT_TOPIC_H */
```

- [ ] **Step 4: Create `esp-libs/mqtt_topic/mqtt_topic.c`**
```c
#include "mqtt_topic.h"
#include <string.h>

int mqtt_topic_join(char *buf, size_t n, const char *const *parts, size_t count) {
    if (buf == NULL || n == 0) return -1;
    buf[0] = '\0';
    if (parts == NULL && count > 0) return -1;

    size_t len = 0;
    for (size_t i = 0; i < count; i++) {
        const char *p = parts[i];
        if (p == NULL) { buf[0] = '\0'; return -1; }
        if (i > 0) {
            if (len + 1 >= n) { buf[0] = '\0'; return -1; }
            buf[len++] = '/';
        }
        size_t pl = strlen(p);
        if (len + pl >= n) { buf[0] = '\0'; return -1; }
        memcpy(buf + len, p, pl);
        len += pl;
    }
    buf[len] = '\0';
    return (int)len;
}

bool mqtt_topic_match(const char *filter, const char *topic) {
    if (filter == NULL || topic == NULL) return false;

    for (;;) {
        /* '#' as the whole remaining filter level matches all remaining topic levels */
        if (filter[0] == '#' && filter[1] == '\0') return true;

        /* extract this level's extent for filter and topic */
        const char *fend = filter; while (*fend && *fend != '/') fend++;
        const char *tend = topic;  while (*tend && *tend != '/') tend++;

        if (filter[0] == '+' && fend == filter + 1) {
            /* '+' matches exactly this one topic level (any content) */
        } else {
            size_t fl = (size_t)(fend - filter);
            size_t tl = (size_t)(tend - topic);
            if (fl != tl || (fl != 0 && memcmp(filter, topic, fl) != 0)) return false;
        }

        filter = fend;
        topic  = tend;

        if (*filter == '/' && *topic == '/') { filter++; topic++; continue; }
        if (*filter == '\0' && *topic == '\0') return true;
        /* topic exhausted but filter has more: only "/#" (parent match) succeeds */
        if (*filter == '/' && *topic == '\0') {
            return (filter[1] == '#' && filter[2] == '\0');
        }
        return false; /* level-count mismatch */
    }
}
```

- [ ] **Step 5: Run, expect PASS** — `cd /Users/man.nk/git/clibs/esp-libs && make mqtt_topic`
Expected: `7 Tests 0 Failures 0 Ignored` / `OK`. Paste output.

- [ ] **Step 6: Confirm strict** — `cd /Users/man.nk/git/clibs/esp-libs && cc -std=c11 -Wall -Wextra -Werror -Imqtt_topic -c mqtt_topic/mqtt_topic.c -o /dev/null && echo STRICT_OK`

- [ ] **Step 7: Commit**
```bash
cd /Users/man.nk/git/clibs
git add esp-libs/mqtt_topic
git commit -m "feat(esp-libs/mqtt_topic): topic join + MQTT wildcard match + tests"
```

---

## Task 5: Final gates + README

**Files:** Create `esp-libs/README.md`.

- [ ] **Step 1: Run all gates**
```bash
cd /Users/man.nk/git/clibs/esp-libs
make clean && make test    # 4 suites: backoff 4, debounce 4, filter 4, mqtt_topic 7 — all 0 Failures
make strict                # strict: OK
```
Paste the four suite summary lines + `strict: OK`. If anything fails, STOP and report BLOCKED.

- [ ] **Step 2: Create `esp-libs/README.md`**
```markdown
# esp-libs

Reusable C building blocks for **ESP8266** smart-home nodes (ESP8266 RTOS SDK,
FreeRTOS). Built bottom-up:

- **Layer 1 — pure logic (this set):** plain portable C, host-unit-tested with
  Unity, no SDK/hardware needed (compiles on ESP8266 unchanged).
- *Layer 2 — SDK wrappers* (wifi / mqtt-client / nvs) — later, needs the SDK.
- *Layer 3 — device drivers* (DHT, DS18B20, relay…) — later, needs hardware.

## Layer-1 libraries

| Lib | What it does |
|-----|--------------|
| `backoff` | Exponential reconnect backoff for WiFi/MQTT (`backoff_init/next/reset`). |
| `debounce` | Input debounce with edge detection (`debounce_init/update/level`). |
| `filter` | Integer EMA smoothing for ADC (`ema_init/update`, alpha = 1/2^shift, no floats). |
| `mqtt_topic` | Topic join + MQTT `+`/`#` wildcard match (`mqtt_topic_join/match`). |

Each is freestanding (`<stdint.h>`/`<stdbool.h>`/`<stddef.h>`, +`<string.h>` in
`mqtt_topic`), static / no-malloc, transparent struct + `self`-methods.

## Tests

Host unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity), reused from
`../common/third_party/unity`.

\```sh
make test     # build + run all four suites
make strict   # -Werror warning gate
make clean
\```

## License

MIT — see the workspace `common/LICENSE`.
```
NOTE: when writing the file, use real triple backticks for the ```sh fence (the `\``` above is escaped only for this plan).

- [ ] **Step 3: Commit**
```bash
cd /Users/man.nk/git/clibs
git add esp-libs/README.md
git commit -m "docs(esp-libs): README for Layer-1 pure building blocks"
```

- [ ] **Step 4: STOP — controller handles integration.** Do NOT merge/push. Report DONE so the controller runs the final review, merges the feature branch into clibs main, and pushes.

---

## Self-Review notes
- **Spec coverage:** scaffold (Makefile reuse-common-Unity, .gitignore) → Task 1; backoff → T1; debounce → T2; filter/EMA → T3; mqtt_topic (join+match) → T4; README + `make test`/`strict` → T5. No ESP cross-compile gate (spec defers to Layer 2). ✓
- **Interfaces / type consistency:** `backoff`/`backoff_*`, `debounce`/`debounce_edge`/`debounce_*`, `ema`/`ema_*`, `mqtt_topic_join`/`mqtt_topic_match` are identical across each header, impl, and test. ✓
- **Placeholder scan:** none — full code in every step.
- **Test values double-checked:** backoff 100→1000 caps at 1000; EMA shift=2 gives 25 then 43; join "home/kitchen/temp"=17 chars; match `home/#` matches `home`. ✓
- **No-malloc / freestanding:** every lib uses only the allowed headers; no malloc. ✓
- **esp-libs committed directly in clibs** (not a submodule); reuses `../common/third_party/unity`.
