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
