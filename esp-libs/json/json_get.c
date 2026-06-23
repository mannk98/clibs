#include "json_get.h"

#include <string.h>

#include "cJSON.h"

static int copy_into(char *buf, size_t n, const char *src)
{
    size_t len = strlen(src);
    if (len >= n) {
        len = n - 1;            /* truncate to fit */
    }
    memcpy(buf, src, len);
    buf[len] = '\0';
    return (int) len;
}

int json_get_str(const char *json, const char *key, char *buf, size_t n, const char *def)
{
    if (json == NULL || key == NULL || buf == NULL || n == 0) {
        return -1;
    }
    const char *result = (def != NULL) ? def : "";
    cJSON *root = cJSON_Parse(json);
    if (root != NULL) {
        cJSON *item = cJSON_GetObjectItem(root, key);
        if (item != NULL && cJSON_IsString(item) && item->valuestring != NULL) {
            result = item->valuestring;
        }
        int len = copy_into(buf, n, result);
        cJSON_Delete(root);
        return len;
    }
    return copy_into(buf, n, result);
}

int json_get_int(const char *json, const char *key, int def)
{
    if (json == NULL || key == NULL) {
        return def;
    }
    int result = def;
    cJSON *root = cJSON_Parse(json);
    if (root != NULL) {
        cJSON *item = cJSON_GetObjectItem(root, key);
        if (item != NULL && cJSON_IsNumber(item)) {
            result = item->valueint;
        }
        cJSON_Delete(root);
    }
    return result;
}
