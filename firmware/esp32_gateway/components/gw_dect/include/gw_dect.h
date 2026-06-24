#pragma once

#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*gw_dect_rx_cb_t)(const char *json, size_t len, void *ctx);

esp_err_t gw_dect_init(void);
esp_err_t gw_dect_restart(void);

void gw_dect_set_rx_callback(gw_dect_rx_cb_t cb, void *ctx);

esp_err_t gw_dect_send_json(const char *json, size_t len);
esp_err_t gw_dect_healthcheck(void);

/**
 * @brief Inject a line as if received from nRF9151 (test/debug).
 *
 * The line must be plain modem output text without CRLF.
 */
esp_err_t gw_dect_test_inject_rx_line(const char *line);

#ifdef __cplusplus
}
#endif
