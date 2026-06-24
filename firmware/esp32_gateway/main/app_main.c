/*
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "gw_bridge.h"
#include "gw_dect.h"
#include "gw_diag.h"
#include "gw_eth.h"
#include "gw_watchdog.h"

static const char *TAG = "app_main";

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    init_nvs();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Starting Ethernet link");
    ESP_ERROR_CHECK(gw_eth_init());

    ESP_LOGI(TAG, "Starting DECT host link");
    ESP_ERROR_CHECK(gw_dect_init());

    // Mandatory minimal AT validation on boot.
    esp_err_t at_ok = gw_dect_healthcheck();
    if (at_ok != ESP_OK) {
        ESP_LOGW(TAG, "Initial AT check failed: %s", esp_err_to_name(at_ok));
    } else {
        ESP_LOGI(TAG, "AT link validated");
    }

    ESP_LOGI(TAG, "Starting bridge tasks");
    ESP_ERROR_CHECK(gw_bridge_start());

    ESP_LOGI(TAG, "Starting watchdog supervisor");
    ESP_ERROR_CHECK(gw_watchdog_start());

    ESP_LOGI(TAG, "Starting USB debug and example traffic");
    ESP_ERROR_CHECK(gw_diag_start());

    ESP_LOGI(TAG, "Gateway ready: Ethernet <-> DECT NR+");
}
*/
