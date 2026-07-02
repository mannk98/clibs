/* ota_version — pure semver parse/compare. See ota_version.h. */
#include "ota_version.h"
#include <stddef.h> /* NULL */

/* Parse one decimal component from *pp, advancing *pp past the digits.
 * Requires at least one digit; rejects values > 65535 (accumulated on uint32
 * to avoid overflow — no left-shift of a negative value). Returns true and
 * sets *value on success; on failure returns false and *pp is unspecified. */
static bool parse_component(const char **pp, uint16_t *value) {
    const char *p = *pp;
    uint32_t acc = 0;
    int ndigits = 0;

    while (*p >= '0' && *p <= '9') {
        acc = acc * 10u + (uint32_t)(*p - '0');
        if (acc > 65535u) {
            return false; /* component exceeds uint16_t range */
        }
        ndigits++;
        p++;
    }
    if (ndigits == 0) {
        return false; /* no digits (e.g. empty, or non-digit char) */
    }
    *value = (uint16_t)acc;
    *pp = p;
    return true;
}

bool ota_version_parse(const char *s, ota_version *out) {
    if (s == NULL || out == NULL) {
        return false;
    }

    ota_version tmp = {0, 0, 0};
    const char *p = s;

    if (!parse_component(&p, &tmp.major)) {
        return false;
    }
    if (*p != '.') {
        return false;
    }
    p++;
    if (!parse_component(&p, &tmp.minor)) {
        return false;
    }
    if (*p != '.') {
        return false;
    }
    p++;
    if (!parse_component(&p, &tmp.patch)) {
        return false;
    }
    if (*p != '\0') {
        return false; /* trailing characters (e.g. a 4th component) */
    }

    *out = tmp;
    return true;
}

int ota_version_cmp(ota_version a, ota_version b) {
    if (a.major != b.major) {
        return a.major < b.major ? -1 : 1;
    }
    if (a.minor != b.minor) {
        return a.minor < b.minor ? -1 : 1;
    }
    if (a.patch != b.patch) {
        return a.patch < b.patch ? -1 : 1;
    }
    return 0;
}

bool ota_version_is_newer(ota_version current, ota_version candidate) {
    return ota_version_cmp(candidate, current) > 0;
}
