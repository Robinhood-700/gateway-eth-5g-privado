#pragma once

#include <stdbool.h>
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

esp_err_t gw_bridge_start(void);
esp_err_t gw_bridge_get_stats(gw_bridge_stats_t *out_stats);
void gw_bridge_reset_stats(void);

/**
 * @brief Runtime peer override for DECT->Ethernet path.
 *
 * Sets fixed destination IP/port where DECT-originated JSON is sent over UDP.
 */
esp_err_t gw_bridge_set_runtime_peer(const char *ipv4, uint16_t port);

/**
 * @brief Inject JSON as if it arrived from DECT side.
 *
 * Useful for USB debug/E2E testing when no real modem payload is available.
 */
esp_err_t gw_bridge_inject_dect_json(const char *json, size_t len);

#ifdef __cplusplus
}
#endif
