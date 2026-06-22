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
