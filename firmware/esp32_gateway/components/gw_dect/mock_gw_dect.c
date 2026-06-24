#include "gw_dect.h"

#include <string.h>

static gw_dect_rx_cb_t mock_rx_cb = NULL;
static void *mock_rx_ctx = NULL;

esp_err_t mock_gw_dect_init(void)
{
    return ESP_OK;
}

esp_err_t mock_gw_dect_restart(void)
{
    return ESP_OK;
}

void mock_gw_dect_set_rx_callback(gw_dect_rx_cb_t cb, void *ctx)
{
    mock_rx_cb = cb;
    mock_rx_ctx = ctx;
}

esp_err_t mock_gw_dect_send_json(const char *json, size_t len)
{
    if (json == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t mock_gw_dect_healthcheck(void)
{
    return ESP_OK;
}

esp_err_t mock_gw_dect_test_inject_rx_line(const char *line)
{
    if (line == NULL || line[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    if (mock_rx_cb != NULL) {
        mock_rx_cb(line, strlen(line), mock_rx_ctx);
    }

    return ESP_OK;
}
