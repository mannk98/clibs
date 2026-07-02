#ifndef CLIBS_OTA_MANIFEST_H
#define CLIBS_OTA_MANIFEST_H

#include <stdint.h>
#include <stdbool.h>

/* ota_manifest — pure OTA manifest validator (L3 pure part).
 *
 * Layer: host-testable, freestanding (only <stdint.h>/<stdbool.h>; <string.h>
 * used in the .c for strlen/strncmp). No SDK headers. Links ota_version.c
 * because it delegates version validation to ota_version_parse.
 *
 * A manifest describes a candidate firmware image announced over the network.
 * Before an OTA download is started, the manifest fields are validated to reject
 * obviously malformed or oversized announcements without touching flash.
 *
 * Conventions: no malloc, NULL-guarded, reject-path validation. */

/* True iff every field of an announced OTA manifest is well-formed:
 *
 *   - version    : parses as a full "MAJOR.MINOR.PATCH" semver
 *                  (ota_version_parse(version) succeeds);
 *   - url        : begins with the literal "http://" (7 chars) AND has at least
 *                  one more character after it (url[7] != '\0');
 *   - size       : 0 < size <= max_size;
 *   - sha256_hex : exactly 64 characters, all lowercase hex ([0-9a-f]).
 *
 * Any NULL pointer argument (version, url, or sha256_hex) -> false. */
bool ota_manifest_valid(const char *version, const char *url,
                        uint32_t size, const char *sha256_hex,
                        uint32_t max_size);

#endif /* CLIBS_OTA_MANIFEST_H */
