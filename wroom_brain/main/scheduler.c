#include <stdbool.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "scheduler.h"

static const char *TAG = "scheduler";
static int64_t next_status_us = 0;

void scheduler_init(void)
{
    next_status_us = esp_timer_get_time() + 30LL * 1000LL * 1000LL;
    ESP_LOGI(TAG, "Autonomy scheduler initialized");
}

void scheduler_tick(incoming_cb_t dispatch_cb)
{
    if (!dispatch_cb) {
        return;
    }

    int64_t now = esp_timer_get_time();
    if (now >= next_status_us) {
        dispatch_cb("status");
        next_status_us = now + 30LL * 1000LL * 1000LL;
    }
}
