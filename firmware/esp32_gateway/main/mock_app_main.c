#include <stdio.h>

typedef int esp_err_t;

#define ESP_OK 0
#define ESP_LOGI(tag, fmt, ...) printf("[INFO] %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("[WARN] %s: " fmt "\n", tag, ##__VA_ARGS__)

static const char *TAG = "mock_app_main";

static esp_err_t mock_init_nvs(void)
{
    return ESP_OK;
}

static esp_err_t mock_esp_netif_init(void)
{
    return ESP_OK;
}

static esp_err_t mock_esp_event_loop_create_default(void)
{
    return ESP_OK;
}

static esp_err_t mock_gw_eth_init(void)
{
    return ESP_OK;
}

static esp_err_t mock_gw_dect_init(void)
{
    return ESP_OK;
}

static esp_err_t mock_gw_dect_healthcheck(void)
{
    return ESP_OK;
}

static esp_err_t mock_gw_bridge_start(void)
{
    return ESP_OK;
}

static esp_err_t mock_gw_watchdog_start(void)
{
    return ESP_OK;
}

static esp_err_t mock_gw_diag_start(void)
{
    return ESP_OK;
}


void mock_app_main(void)
{
    mock_init_nvs();

    mock_esp_netif_init();
    mock_esp_event_loop_create_default();

    ESP_LOGI(TAG, "Starting Ethernet link");
    mock_gw_eth_init();

    ESP_LOGI(TAG, "Starting DECT host link");
    mock_gw_dect_init();

    esp_err_t at_ok = mock_gw_dect_healthcheck();

    if (at_ok != ESP_OK) {
        ESP_LOGW(TAG, "Initial AT check failed");
    } else {
        ESP_LOGI(TAG, "AT link validated");
    }

    ESP_LOGI(TAG, "Starting bridge tasks");
    mock_gw_bridge_start();

    ESP_LOGI(TAG, "Starting watchdog supervisor");
    mock_gw_watchdog_start();

    ESP_LOGI(TAG, "Starting USB debug and example traffic");
    mock_gw_diag_start();

    ESP_LOGI(TAG, "Gateway ready: Ethernet <-> DECT NR+");
}

void app_main(void)
{
    mock_app_main();
}
