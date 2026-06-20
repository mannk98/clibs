#ifndef CLIBS_STR_UTILS_H
#define CLIBS_STR_UTILS_H

#include <stdint.h>

/* Freestanding string helpers — no libc <string.h> dependency (MCU-friendly). */

/* Length of NUL-terminated string s (0 if s is NULL).
 * Max 65535 chars; wraps silently on longer strings (uint16_t length). */
uint16_t str_len(const char *s);

/* strncmp-style: compare up to n bytes. Returns 0 if equal, else the signed
 * difference of the first differing byte (compared as unsigned char). No
 * truncation. a and b must be non-NULL. */
int str_compare_n(const char *a, const char *b, uint16_t n);

/* Bounded copy: copies at most n bytes from src to dst INCLUDING the terminating
 * NUL; dst is always NUL-terminated when n > 0. Returns dst. (Like strncpy but
 * guarantees termination; does NOT return a length, so it can't report truncation.) */
char *str_cpy_n(char *dst, const char *src, uint16_t n);

/* Reverse s in place (uses str_len, not libc strlen). No-op if s is NULL or len<2. */
void str_reverse(char *s);

#endif /* CLIBS_STR_UTILS_H */
