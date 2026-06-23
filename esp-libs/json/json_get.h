#ifndef CLIBS_JSON_GET_H
#define CLIBS_JSON_GET_H

#include <stddef.h>

/* Parse `json`, read string field `key` into buf[n] (NUL-term, truncated to fit).
 * Missing / not-a-string / parse-fail -> copy `def` (NULL->""), return its length.
 * On success return the copied length. -1 on NULL json|key|buf or n==0. */
int json_get_str(const char *json, const char *key, char *buf, size_t n, const char *def);

/* Parse `json`, read integer field `key`. Missing / not-a-number / parse-fail -> def. */
int json_get_int(const char *json, const char *key, int def);

#endif /* CLIBS_JSON_GET_H */
