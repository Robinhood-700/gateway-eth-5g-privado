#include "gw_watchdog.h"

#include <stdint.h>

#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gw_dect.h"
#include "gw_eth.h"

static const char *TAG = "gw_wd";

static void supervisor_task(void *arg)
{
    (void)arg;

    esp_task_wdt_add(NULL);

    uint32_t dect_fail_count = 0;

    while (1) {
        int64_t down_ms = gw_eth_link_down_ms();
        if (down_ms > ((int64_t)CONFIG_GW_ETH_RESTART_SEC * 1000LL)) {
            ESP_LOGW(TAG, "Ethernet down for %lld ms -> restart", down_ms);
            gw_eth_restart();
        }

        if (gw_dect_healthcheck() != ESP_OK) {
            dect_fail_count++;
            ESP_LOGW(TAG, "DECT AT healthcheck failed (%lu)", (unsigned long)dect_fail_count);
            if (dect_fail_count >= 2) {
                gw_dect_restart();
                dect_fail_count = 0;
            }
        } else {
            dect_fail_count = 0;
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(CONFIG_GW_SUPERVISOR_PERIOD_MS));
    }
}

esp_err_t gw_watchdog_start(void)
{
    BaseType_t ok = xTaskCreate(supervisor_task, "gw_supervisor", 4096, NULL, 8, NULL);
    if (ok != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
