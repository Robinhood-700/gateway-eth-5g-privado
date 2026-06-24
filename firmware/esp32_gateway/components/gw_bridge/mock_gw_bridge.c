#include "gw_bridge.h"

#include <string.h>

static gw_bridge_stats_t mock_stats;

static void mock_stats_reset(void)
{
    memset(&mock_stats, 0, sizeof(mock_stats));
}

esp_err_t mock_gw_bridge_start(void)
{
    mock_stats_reset();
    return ESP_OK;
}

esp_err_t mock_gw_bridge_get_stats(gw_bridge_stats_t *out_stats)
{
    if (out_stats == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_stats = mock_stats;
    return ESP_OK;
}

void mock_gw_bridge_reset_stats(void)
{
    mock_stats_reset();
}

esp_err_t mock_gw_bridge_set_runtime_peer(const char *ipv4, uint16_t port)
{
    if (ipv4 == NULL || ipv4[0] == '\0' || port == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t mock_gw_bridge_inject_dect_json(const char *json, size_t len)
{
    if (json == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    mock_stats.dect_rx_packets++;
    mock_stats.dect_to_eth_packets++;

    return ESP_OK;
}
