#include "gw_eth.h"

#include <string.h>
#include <sys/time.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "esp_eth_mac_w5500.h"
#include "esp_eth_phy.h"
#include "esp_eth_phy_w5500.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "gw_eth";

static esp_eth_handle_t s_eth_handle;
static esp_netif_t *s_eth_netif;
static esp_eth_netif_glue_handle_t s_eth_glue;

static bool s_link_up;
static bool s_have_ip;
static int64_t s_link_down_since_ms;

static int64_t now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec * 1000LL) + (tv.tv_usec / 1000);
}

static void on_eth_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    (void)data;

    switch (id) {
    case ETHERNET_EVENT_CONNECTED:
        s_link_up = true;
        s_link_down_since_ms = 0;
        ESP_LOGI(TAG, "Ethernet link up");
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        s_link_up = false;
        s_have_ip = false;
        s_link_down_since_ms = now_ms();
        ESP_LOGW(TAG, "Ethernet link down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet started");
        break;
    case ETHERNET_EVENT_STOP:
        s_link_up = false;
        s_have_ip = false;
        s_link_down_since_ms = now_ms();
        ESP_LOGW(TAG, "Ethernet stopped");
        break;
    default:
        break;
    }
}

static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;

    if (id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        s_have_ip = true;
        ESP_LOGI(TAG, "Ethernet got IP: " IPSTR, IP2STR(&evt->ip_info.ip));
    } else if (id == IP_EVENT_ETH_LOST_IP) {
        s_have_ip = false;
        ESP_LOGW(TAG, "Ethernet lost IP");
    }
}

static esp_err_t install_eth_driver(void)
{
    if (s_eth_handle != NULL) {
        return ESP_OK;
    }

    spi_bus_config_t buscfg = {
        .mosi_io_num = CONFIG_GW_ETH_SPI_MOSI_GPIO,
        .miso_io_num = CONFIG_GW_ETH_SPI_MISO_GPIO,
        .sclk_io_num = CONFIG_GW_ETH_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1536,
    };

    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG,
                        "spi_bus_initialize failed");

    spi_device_interface_config_t devcfg = {
        .command_bits = 16,
        .address_bits = 8,
        .mode = 0,
        .clock_speed_hz = CONFIG_GW_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num = CONFIG_GW_ETH_SPI_CS_GPIO,
        .queue_size = 20,
    };

    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(SPI2_HOST, &devcfg);
    w5500_config.int_gpio_num = CONFIG_GW_ETH_INT_GPIO;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.reset_gpio_num = CONFIG_GW_ETH_RST_GPIO;

    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    ESP_RETURN_ON_FALSE(mac != NULL, ESP_ERR_NO_MEM, TAG, "esp_eth_mac_new_w5500 failed");

    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
    if (phy == NULL) {
        mac->del(mac);
        return ESP_ERR_NO_MEM;
    }

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_err_t err = esp_eth_driver_install(&eth_config, &s_eth_handle);
    if (err != ESP_OK) {
        mac->del(mac);
        phy->del(phy);
        return err;
    }

    return ESP_OK;
}

esp_err_t gw_eth_init(void)
{
    if (s_eth_handle != NULL) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &on_eth_event, NULL), TAG,
        "ETH_EVENT register failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &on_ip_event, NULL), TAG,
        "IP_EVENT_ETH_GOT_IP register failed");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_LOST_IP, &on_ip_event, NULL), TAG,
        "IP_EVENT_ETH_LOST_IP register failed");

    ESP_RETURN_ON_ERROR(install_eth_driver(), TAG, "install_eth_driver failed");

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    s_eth_netif = esp_netif_new(&cfg);
    ESP_RETURN_ON_FALSE(s_eth_netif != NULL, ESP_ERR_NO_MEM, TAG, "esp_netif_new failed");

    s_eth_glue = esp_eth_new_netif_glue(s_eth_handle);
    ESP_RETURN_ON_FALSE(s_eth_glue != NULL, ESP_ERR_NO_MEM, TAG, "esp_eth_new_netif_glue failed");

    ESP_RETURN_ON_ERROR(esp_netif_attach(s_eth_netif, s_eth_glue), TAG, "esp_netif_attach failed");
    ESP_RETURN_ON_ERROR(esp_eth_start(s_eth_handle), TAG, "esp_eth_start failed");

    s_link_down_since_ms = now_ms();
    ESP_LOGI(TAG, "W5500 initialized");
    return ESP_OK;
}

esp_err_t gw_eth_restart(void)
{
    if (s_eth_handle == NULL) {
        return gw_eth_init();
    }

    ESP_LOGW(TAG, "Restarting Ethernet driver");
    ESP_RETURN_ON_ERROR(esp_eth_stop(s_eth_handle), TAG, "esp_eth_stop failed");
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_RETURN_ON_ERROR(esp_eth_start(s_eth_handle), TAG, "esp_eth_start failed");
    return ESP_OK;
}

bool gw_eth_link_up(void)
{
    return s_link_up;
}

bool gw_eth_has_ip(void)
{
    return s_have_ip;
}

int64_t gw_eth_link_down_ms(void)
{
    if (s_link_up || s_link_down_since_ms == 0) {
        return 0;
    }
    return now_ms() - s_link_down_since_ms;
}

esp_err_t gw_eth_get_ipv4(esp_ip4_addr_t *out_ip)
{
    if (s_eth_netif == NULL || out_ip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_netif_ip_info_t info;
    ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(s_eth_netif, &info), TAG, "esp_netif_get_ip_info failed");
    *out_ip = info.ip;
    return ESP_OK;
}
