#include <stdbool.h>

#include "esp_log.h"

#include "transport_telegram.h"

static const char *TAG = "transport_tg";

void transport_telegram_init(void)
{
    ESP_LOGI(TAG, "Telegram transport stub initialized");
    ESP_LOGI(TAG, "TODO: Wi-Fi + HTTPS long polling implementation");
}

void transport_telegram_poll(incoming_cb_t cb)
{
    (void)cb;
}

void transport_telegram_send(const char *msg)
{
    ESP_LOGI(TAG, "TX: %s", msg ? msg : "");
}
