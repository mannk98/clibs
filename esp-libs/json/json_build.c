#include "json_build.h"

#include <stdio.h>
#include <string.h>

/* Append n bytes; set sticky err on overflow (needs room for the trailing NUL). */
static void append(json_build *b, const char *s, size_t n)
{
    if (b->err) {
        return;
    }
    if (b->len + n >= b->cap) {       /* need buf[len+n] for NUL too */
        b->err = true;
        return;
    }
    memcpy(b->buf + b->len, s, n);
    b->len += n;
    b->buf[b->len] = '\0';
}

/* Append a quoted JSON string value, escaping " and \. */
static void append_escaped(json_build *b, const char *s)
{
    append(b, "\"", 1);
    for (const char *p = s; *p != '\0' && !b->err; p++) {
        if (*p == '"' || *p == '\\') {
            char esc[2] = { '\\', *p };
            append(b, esc, 2);
        } else {
            append(b, p, 1);
        }
    }
    append(b, "\"", 1);
}

static void begin_member(json_build *b, const char *key)
{
    if (!b->first) {
        append(b, ",", 1);
    }
    b->first = false;
    append_escaped(b, key);
    append(b, ":", 1);
}

void json_build_init(json_build *b, char *buf, size_t cap)
{
    if (b == NULL) {
        return;
    }
    b->buf = buf;
    b->cap = cap;
    b->len = 0;
    b->first = true;
    b->err = (buf == NULL || cap == 0);
    if (!b->err) {
        b->buf[0] = '\0';
        append(b, "{", 1);
    }
}

void json_build_str(json_build *b, const char *key, const char *val)
{
    if (b == NULL) {
        return;
    }
    if (key == NULL || val == NULL) {
        b->err = true;
        return;
    }
    begin_member(b, key);
    append_escaped(b, val);
}

void json_build_int(json_build *b, const char *key, int32_t val)
{
    if (b == NULL) {
        return;
    }
    if (key == NULL) {
        b->err = true;
        return;
    }
    begin_member(b, key);
    char num[12];   /* fits -2147483648 + NUL */
    int n = snprintf(num, sizeof num, "%ld", (long) val);
    if (n < 0) {
        b->err = true;
        return;
    }
    append(b, num, (size_t) n);
}

void json_build_raw(json_build *b, const char *key, const char *raw)
{
    if (b == NULL) {
        return;
    }
    if (key == NULL || raw == NULL) {
        b->err = true;
        return;
    }
    begin_member(b, key);
    append(b, raw, strlen(raw));
}

int json_build_end(json_build *b)
{
    if (b == NULL) {
        return -1;
    }
    append(b, "}", 1);
    return b->err ? -1 : (int) b->len;
}
