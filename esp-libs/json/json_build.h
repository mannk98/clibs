#ifndef CLIBS_JSON_BUILD_H
#define CLIBS_JSON_BUILD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* No-malloc JSON object builder into a caller buffer:
 *   json_build b; json_build_init(&b, buf, sizeof buf);
 *   json_build_str(&b, "state", "on"); json_build_int(&b, "rssi", -60);
 *   int len = json_build_end(&b);   // len >= 0, or -1 on overflow
 * Pure C, no SDK headers (host-testable). */
typedef struct {
    char  *buf;
    size_t cap;     /* buffer capacity */
    size_t len;     /* current length (excl NUL) */
    bool   err;     /* sticky overflow/arg error */
    bool   first;   /* no comma before the first member */
} json_build;

void json_build_init(json_build *b, char *buf, size_t cap);          /* writes "{" */
void json_build_str(json_build *b, const char *key, const char *val);/* "key":"val" (escapes " and \) */
void json_build_int(json_build *b, const char *key, int32_t val);    /* "key":123 */
void json_build_raw(json_build *b, const char *key, const char *raw);/* "key":raw (caller-formed) */
int  json_build_end(json_build *b);   /* writes "}"; returns len (excl NUL), or -1 if err/overflow */

#endif /* CLIBS_JSON_BUILD_H */
