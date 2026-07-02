/* ota_manifest — pure OTA manifest validator. See ota_manifest.h. */
#include "ota_manifest.h"
#include "ota_version.h"

#include <stddef.h> /* NULL */
#include <string.h> /* strlen, strncmp */

/* True iff c is a lowercase hex digit [0-9a-f]. */
static bool is_lower_hex(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

bool ota_manifest_valid(const char *version, const char *url,
                        uint32_t size, const char *sha256_hex,
                        uint32_t max_size) {
    if (version == NULL || url == NULL || sha256_hex == NULL) {
        return false;
    }

    /* version must parse as a full "MAJOR.MINOR.PATCH" semver. */
    ota_version parsed;
    if (!ota_version_parse(version, &parsed)) {
        return false;
    }

    /* url must start with "http://" and have at least one more char after it. */
    if (strncmp(url, "http://", 7) != 0 || url[7] == '\0') {
        return false;
    }

    /* size must be within (0, max_size]. */
    if (size == 0u || size > max_size) {
        return false;
    }

    /* sha256_hex must be exactly 64 lowercase-hex chars. */
    if (strlen(sha256_hex) != 64u) {
        return false;
    }
    for (size_t i = 0; i < 64u; i++) {
        if (!is_lower_hex(sha256_hex[i])) {
            return false;
        }
    }

    return true;
}
