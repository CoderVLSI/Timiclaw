#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "tool_registry.h"

static const char *TAG = "tool_registry";

void tool_registry_init(void)
{
    ESP_LOGI(TAG, "Allowed tools: status, relay_set <pin> <0|1>, sensor_read <id>");
}

static bool starts_with(const char *s, const char *prefix)
{
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

bool tool_registry_execute(const char *input, char *out, size_t out_len)
{
    if (!input || !out || out_len == 0) {
        return false;
    }

    if (strcmp(input, "status") == 0) {
        snprintf(out, out_len, "OK: uptime and health nominal");
        return true;
    }

    if (starts_with(input, "relay_set ")) {
        int pin = -1;
        int state = -1;
        if (sscanf(input, "relay_set %d %d", &pin, &state) == 2) {
            if ((pin >= 0 && pin <= 39) && (state == 0 || state == 1)) {
                snprintf(out, out_len, "OK: relay pin %d -> %d", pin, state);
                return true;
            }
        }
        snprintf(out, out_len, "ERR: usage relay_set <pin> <0|1>");
        return true;
    }

    if (starts_with(input, "sensor_read ")) {
        int id = -1;
        if (sscanf(input, "sensor_read %d", &id) == 1 && id >= 0 && id <= 16) {
            snprintf(out, out_len, "OK: sensor %d value=0", id);
            return true;
        }
        snprintf(out, out_len, "ERR: usage sensor_read <id>");
        return true;
    }

    return false;
}
