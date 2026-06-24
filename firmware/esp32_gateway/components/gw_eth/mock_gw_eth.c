#include "gw_eth.h"

#include <string.h>

static bool mock_link_up = true;
static bool mock_have_ip = true;
static int64_t mock_link_down_ms = 0;

esp_err_t mock_gw_eth_init(void)
{
    mock_link_up = true;
    mock_have_ip = true;
    mock_link_down_ms = 0;

    return ESP_OK;
}

esp_err_t mock_gw_eth_restart(void)
{
    mock_link_up = true;
    mock_have_ip = true;
    mock_link_down_ms = 0;

    return ESP_OK;
}

bool mock_gw_eth_link_up(void)
{
    return mock_link_up;
}

bool mock_gw_eth_has_ip(void)
{
    return mock_have_ip;
}

int64_t mock_gw_eth_link_down_ms(void)
{
    return mock_link_down_ms;
}

esp_err_t mock_gw_eth_get_ipv4(esp_ip4_addr_t *out_ip)
{
    if (out_ip == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out_ip, 0, sizeof(*out_ip));

    /* 192.168.1.100 */
    out_ip->addr =
        ((uint32_t)192) |
        ((uint32_t)168 << 8) |
        ((uint32_t)1 << 16) |
        ((uint32_t)100 << 24);

    return ESP_OK;
}
