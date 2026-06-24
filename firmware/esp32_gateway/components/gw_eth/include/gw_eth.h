#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_netif_ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t gw_eth_init(void);
esp_err_t gw_eth_restart(void);

bool gw_eth_link_up(void);
bool gw_eth_has_ip(void);
int64_t gw_eth_link_down_ms(void);

esp_err_t gw_eth_get_ipv4(esp_ip4_addr_t *out_ip);

#ifdef __cplusplus
}
#endif
