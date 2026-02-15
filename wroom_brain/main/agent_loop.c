#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "esp_log.h"

#include "agent_loop.h"
#include "scheduler.h"
#include "tool_registry.h"
#include "transport_telegram.h"

static const char *TAG = "agent_loop";

static void on_incoming_message(const char *msg)
{
    if (msg == NULL || msg[0] == '\0') {
        return;
    }

    ESP_LOGI(TAG, "Incoming message: %s", msg);

    char response[160] = {0};
    bool ok = tool_registry_execute(msg, response, sizeof(response));

    if (!ok) {
        transport_telegram_send("Denied or unknown command");
        return;
    }

    transport_telegram_send(response);
}

void agent_loop_init(void)
{
    tool_registry_init();
    scheduler_init();
    transport_telegram_init();

    ESP_LOGI(TAG, "Agent loop initialized");
    ESP_LOGI(TAG, "Safety model: local allowlist only");
}

void agent_loop_tick(void)
{
    transport_telegram_poll(on_incoming_message);
    scheduler_tick(on_incoming_message);
}
