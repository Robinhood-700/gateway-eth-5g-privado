#include "gw_dect.h"

#include <ctype.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "gw_dect";

#define RX_LINE_MAX 1024

static gw_dect_rx_cb_t s_rx_cb;
static void *s_rx_ctx;
static SemaphoreHandle_t s_at_ok_sem;

static TaskHandle_t s_rx_task;

#if CONFIG_GW_DECT_HOST_IF_SPI
static spi_device_handle_t s_spi;
static bool s_spi_bus_init;
#endif

static void publish_payload_line(const char *line)
{
    if (line == NULL) {
        return;
    }

    while (isspace((unsigned char)*line)) {
        line++;
    }

    if (strncmp(line, "OK", 2) == 0) {
        if (s_at_ok_sem) {
            xSemaphoreGive(s_at_ok_sem);
        }
        return;
    }

    if (strncmp(line, "ERROR", 5) == 0) {
        ESP_LOGW(TAG, "Modem returned ERROR");
        return;
    }

    const char *prefix = CONFIG_GW_DECT_RX_PREFIX;
    bool should_forward = true;
    if (prefix != NULL && prefix[0] != '\0') {
        size_t prefix_len = strlen(prefix);
        if (strncmp(line, prefix, prefix_len) == 0) {
            line += prefix_len;
            while (isspace((unsigned char)*line)) {
                line++;
            }
        } else {
            should_forward = false;
        }
    }

    if (should_forward && line[0] != '\0' && s_rx_cb != NULL) {
        s_rx_cb(line, strlen(line), s_rx_ctx);
    }
}

#if CONFIG_GW_DECT_HOST_IF_UART
static esp_err_t iface_write(const uint8_t *data, size_t len)
{
    int wr = uart_write_bytes(CONFIG_GW_DECT_UART_PORT_NUM, data, len);
    return (wr == (int)len) ? ESP_OK : ESP_FAIL;
}

static void uart_rx_task(void *arg)
{
    (void)arg;

    uint8_t chunk[128];
    char line[RX_LINE_MAX];
    size_t line_len = 0;

    while (1) {
        int rd = uart_read_bytes(CONFIG_GW_DECT_UART_PORT_NUM, chunk, sizeof(chunk), pdMS_TO_TICKS(100));
        if (rd <= 0) {
            continue;
        }

        for (int i = 0; i < rd; ++i) {
            char c = (char)chunk[i];
            if (c == '\r') {
                continue;
            }
            if (c == '\n') {
                if (line_len > 0) {
                    line[line_len] = '\0';
                    publish_payload_line(line);
                    line_len = 0;
                }
                continue;
            }
            if (line_len < (sizeof(line) - 1)) {
                line[line_len++] = c;
            }
        }
    }
}

static esp_err_t iface_init(void)
{
    uart_config_t cfg = {
        .baud_rate = CONFIG_GW_DECT_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    ESP_RETURN_ON_ERROR(uart_driver_install(CONFIG_GW_DECT_UART_PORT_NUM, 2048, 0, 0, NULL, 0), TAG,
                        "uart_driver_install failed");
    ESP_RETURN_ON_ERROR(uart_param_config(CONFIG_GW_DECT_UART_PORT_NUM, &cfg), TAG,
                        "uart_param_config failed");
    ESP_RETURN_ON_ERROR(
        uart_set_pin(CONFIG_GW_DECT_UART_PORT_NUM, CONFIG_GW_DECT_UART_TX_GPIO,
                     CONFIG_GW_DECT_UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE),
        TAG, "uart_set_pin failed");

    xTaskCreate(uart_rx_task, "gw_dect_uart_rx", 4096, NULL, 10, &s_rx_task);
    return ESP_OK;
}

#else

static esp_err_t spi_xfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    spi_transaction_t t = {
        .flags = 0,
        .length = len * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    return spi_device_transmit(s_spi, &t);
}

static esp_err_t iface_write(const uint8_t *data, size_t len)
{
    return spi_xfer(data, NULL, len);
}

/*
 * Assumption for SPI bridge FW on nRF9151:
 * - Full-duplex byte stream.
 * - RX payload is emitted as ASCII lines ending with '\n'.
 * - ESP32 master polls by clocking dummy bytes.
 */
static void spi_rx_task(void *arg)
{
    (void)arg;

    uint8_t tx[CONFIG_GW_DECT_SPI_POLL_BYTES];
    uint8_t rx[CONFIG_GW_DECT_SPI_POLL_BYTES];
    char line[RX_LINE_MAX];
    size_t line_len = 0;

    memset(tx, 0xFF, sizeof(tx));

    while (1) {
        if (spi_xfer(tx, rx, sizeof(rx)) == ESP_OK) {
            for (size_t i = 0; i < sizeof(rx); ++i) {
                char c = (char)rx[i];
                if (c == '\0' || c == (char)0xFF) {
                    continue;
                }
                if (c == '\r') {
                    continue;
                }
                if (c == '\n') {
                    if (line_len > 0) {
                        line[line_len] = '\0';
                        publish_payload_line(line);
                        line_len = 0;
                    }
                    continue;
                }
                if (isprint((unsigned char)c) && line_len < (sizeof(line) - 1)) {
                    line[line_len++] = c;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static esp_err_t iface_init(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = CONFIG_GW_DECT_SPI_MOSI_GPIO,
        .miso_io_num = CONFIG_GW_DECT_SPI_MISO_GPIO,
        .sclk_io_num = CONFIG_GW_DECT_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1024,
    };

    if (!s_spi_bus_init) {
        ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG,
                            "spi_bus_initialize failed");
        s_spi_bus_init = true;
    }

    spi_device_interface_config_t devcfg = {
        .mode = 0,
        .clock_speed_hz = CONFIG_GW_DECT_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num = CONFIG_GW_DECT_SPI_CS_GPIO,
        .queue_size = 2,
    };

    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI3_HOST, &devcfg, &s_spi), TAG,
                        "spi_bus_add_device failed");

    xTaskCreate(spi_rx_task, "gw_dect_spi_rx", 4096, NULL, 10, &s_rx_task);
    return ESP_OK;
}
#endif

static esp_err_t send_line(const char *line)
{
    if (line == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t n = strlen(line);
    if (n == 0) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(iface_write((const uint8_t *)line, n), TAG, "iface_write line failed");
    ESP_RETURN_ON_ERROR(iface_write((const uint8_t *)"\r\n", 2), TAG, "iface_write CRLF failed");
    return ESP_OK;
}

esp_err_t gw_dect_init(void)
{
    if (s_at_ok_sem == NULL) {
        s_at_ok_sem = xSemaphoreCreateBinary();
        ESP_RETURN_ON_FALSE(s_at_ok_sem != NULL, ESP_ERR_NO_MEM, TAG, "xSemaphoreCreateBinary failed");
    }

    gpio_config_t rst_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_GW_DECT_RST_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&rst_cfg), TAG, "gpio_config RST failed");

    gpio_set_level(CONFIG_GW_DECT_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(CONFIG_GW_DECT_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    return iface_init();
}

esp_err_t gw_dect_restart(void)
{
    ESP_LOGW(TAG, "Restarting DECT host link");

    if (s_rx_task != NULL) {
        vTaskDelete(s_rx_task);
        s_rx_task = NULL;
    }

#if CONFIG_GW_DECT_HOST_IF_UART
    uart_driver_delete(CONFIG_GW_DECT_UART_PORT_NUM);
#else
    if (s_spi != NULL) {
        spi_bus_remove_device(s_spi);
        s_spi = NULL;
    }
#endif

    return gw_dect_init();
}

void gw_dect_set_rx_callback(gw_dect_rx_cb_t cb, void *ctx)
{
    s_rx_cb = cb;
    s_rx_ctx = ctx;
}

esp_err_t gw_dect_send_json(const char *json, size_t len)
{
    if (json == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    char line[RX_LINE_MAX];
    const char *prefix = CONFIG_GW_DECT_TX_PREFIX;
    if (prefix != NULL && prefix[0] != '\0') {
        int wr = snprintf(line, sizeof(line), "%s%.*s", prefix, (int)len, json);
        if (wr <= 0 || wr >= (int)sizeof(line)) {
            return ESP_ERR_NO_MEM;
        }
        return send_line(line);
    }

    if (len >= sizeof(line)) {
        return ESP_ERR_NO_MEM;
    }
    memcpy(line, json, len);
    line[len] = '\0';
    return send_line(line);
}

esp_err_t gw_dect_healthcheck(void)
{
    if (s_at_ok_sem == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    while (xSemaphoreTake(s_at_ok_sem, 0) == pdTRUE) {
    }

    ESP_RETURN_ON_ERROR(send_line("AT"), TAG, "AT send failed");

    if (xSemaphoreTake(s_at_ok_sem, pdMS_TO_TICKS(CONFIG_GW_DECT_AT_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t gw_dect_test_inject_rx_line(const char *line)
{
    if (line == NULL || line[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    publish_payload_line(line);
    return ESP_OK;
}
