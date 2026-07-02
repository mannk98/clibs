#ifndef CLIBS_OTA_VERSION_H
#define CLIBS_OTA_VERSION_H

/* ota_version — pure semantic-version parse/compare for OTA update decisions.
 *
 * Layer: L3 pure part (host-testable, no SDK headers). A firmware build string
 * of the form "MAJOR.MINOR.PATCH" (exactly three decimal components, each
 * 0..65535) is parsed into an ota_version, then compared to decide whether a
 * candidate image is newer than the one currently running.
 *
 * Conventions: no malloc, caller-owned storage, NULL-guarded, freestanding
 * (only <stdint.h>/<stdbool.h>). */

#include <stdint.h>
#include <stdbool.h>

/* Transparent struct — caller allocates on the stack. */
typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
} ota_version;

/* Parse a "MAJOR.MINOR.PATCH" string into *out.
 *
 * Accepts exactly three decimal components separated by '.', each in the range
 * 0..65535, with no leading/trailing/interior non-digit characters and no
 * missing or extra components.
 *
 * On success: writes the three components to *out and returns true.
 * On failure (NULL s, NULL out, or malformed/out-of-range input): returns false
 * and does not write to *out. */
bool ota_version_parse(const char *s, ota_version *out);

/* Lexicographic comparison over (major, minor, patch).
 * Returns -1 if a < b, 0 if a == b, +1 if a > b. */
int ota_version_cmp(ota_version a, ota_version b);

/* True iff candidate is strictly newer than current, i.e. cmp(candidate, current) > 0. */
bool ota_version_is_newer(ota_version current, ota_version candidate);

#endif /* CLIBS_OTA_VERSION_H */
