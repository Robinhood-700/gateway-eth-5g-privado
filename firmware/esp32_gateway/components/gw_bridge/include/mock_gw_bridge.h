#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t eth_rx_packets;
    uint64_t eth_to_dect_packets;
    uint64_t eth_to_dect_errors;

    uint64_t dect_rx_packets;
    uint64_t dect_to_eth_packets;
    uint64_t dect_to_eth_errors;
} gw_bridge_stats_t;

esp_err_t mock_gw_bridge_start(void);

esp_err_t mock_gw_bridge_get_stats(
    gw_bridge_stats_t *out_stats);

void mock_gw_bridge_reset_stats(void);

esp_err_t mock_gw_bridge_set_runtime_peer(
    const char *ipv4,
    uint16_t port);

esp_err_t mock_gw_bridge_inject_dect_json(
    const char *json,
    size_t len);

#ifdef __cplusplus
}
#endif
