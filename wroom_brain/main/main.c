#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "agent_loop.h"

static const char *TAG = "wroom_brain";

void app_main(void)
{
    ESP_LOGI(TAG, "Booting WROOM brain scaffold");

    agent_loop_init();

    while (1) {
        agent_loop_tick();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
