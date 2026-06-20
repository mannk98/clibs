#include "str_utils.h"
#include <stddef.h>  /* NULL */

uint16_t str_len(const char *s) {
    uint16_t n = 0;
    if (s == NULL) return 0;
    while (s[n] != '\0') n++;
    return n;
}

int str_compare_n(const char *a, const char *b, uint16_t n) {
    for (uint16_t i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) return (int)ca - (int)cb;
        if (ca == '\0') return 0;
    }
    return 0;
}

char *str_cpy_n(char *dst, const char *src, uint16_t n) {
    if (dst == NULL || n == 0) return dst;
    uint16_t i = 0;
    if (src != NULL) {
        for (; i < (uint16_t)(n - 1) && src[i] != '\0'; i++)
            dst[i] = src[i];
    }
    dst[i] = '\0';
    return dst;
}

void str_reverse(char *s) {
    if (s == NULL) return;
    uint16_t len = str_len(s);
    if (len < 2) return;
    uint16_t i = 0, j = (uint16_t)(len - 1);
    while (i < j) {
        char tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
        i++;
        j--;
    }
}
