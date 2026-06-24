#include "gw_watchdog.h"

#include <stdio.h>

esp_err_t mock_gw_watchdog_start(void)
{
    printf("[MOCK] Watchdog supervisor started\n");
    return ESP_OK;
}
