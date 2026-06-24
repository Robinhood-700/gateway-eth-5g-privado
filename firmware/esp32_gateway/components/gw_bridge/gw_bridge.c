#include "gw_bridge.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "gw_dect.h"
#include "gw_eth.h"
#include "gw_json.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

static const char *TAG = "gw_bridge";

typedef struct {
    size_t len;
    char payload[CONFIG_GW_MAX_PAYLOAD_LEN * 3];
} bridge_msg_t;

static QueueHandle_t s_dect_rx_q;
static SemaphoreHandle_t s_peer_mutex;
static bool s_have_last_peer;
static struct sockaddr_in s_last_peer;

static bool s_use_fixed_peer;
static struct sockaddr_in s_fixed_peer;
static gw_bridge_stats_t s_stats;
static portMUX_TYPE s_stats_lock = portMUX_INITIALIZER_UNLOCKED;

static void stats_inc(uint64_t *field)
{
    portENTER_CRITICAL(&s_stats_lock);
    (*field)++;
    portEXIT_CRITICAL(&s_stats_lock);
}

static void stats_copy(gw_bridge_stats_t *out)
{
    portENTER_CRITICAL(&s_stats_lock);
    *out = s_stats;
    portEXIT_CRITICAL(&s_stats_lock);
}

static void dect_rx_cb(const char *json, size_t len, void *ctx)
{
    (void)ctx;

    if (json == NULL || len == 0 || s_dect_rx_q == NULL) {
        return;
    }

    stats_inc(&s_stats.dect_rx_packets);

    bridge_msg_t msg = { 0 };
    if (len >= sizeof(msg.payload)) {
        len = sizeof(msg.payload) - 1;
    }

    memcpy(msg.payload, json, len);
    msg.payload[len] = '\0';
    msg.len = len;

    if (xQueueSend(s_dect_rx_q, &msg, 0) != pdTRUE) {
        stats_inc(&s_stats.dect_to_eth_errors);
        ESP_LOGW(TAG, "DECT->ETH queue full, dropping packet");
    }
}

static void udp_rx_task(void *arg)
{
    (void)arg;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed: errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in local = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_GW_UDP_LISTEN_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
        ESP_LOGE(TAG, "bind failed: errno=%d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "UDP RX listening on port %d", CONFIG_GW_UDP_LISTEN_PORT);

    uint8_t rx[CONFIG_GW_MAX_PAYLOAD_LEN];
    char json_out[CONFIG_GW_MAX_PAYLOAD_LEN * 3];

    while (1) {
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);
        int rd = recvfrom(sock, rx, sizeof(rx), 0, (struct sockaddr *)&from, &from_len);
        if (rd < 0) {
            ESP_LOGW(TAG, "recvfrom failed: errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        stats_inc(&s_stats.eth_rx_packets);

        if (!s_use_fixed_peer) {
            if (xSemaphoreTake(s_peer_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                s_last_peer = from;
                s_have_last_peer = true;
                xSemaphoreGive(s_peer_mutex);
            }
        }

        size_t json_len = gw_json_ensure(rx, (size_t)rd, json_out, sizeof(json_out));
        if (json_len == 0) {
            stats_inc(&s_stats.eth_to_dect_errors);
            ESP_LOGW(TAG, "Failed to JSON-wrap Ethernet payload len=%d", rd);
            continue;
        }

        esp_err_t err = gw_dect_send_json(json_out, json_len);
        if (err != ESP_OK) {
            stats_inc(&s_stats.eth_to_dect_errors);
            ESP_LOGW(TAG, "gw_dect_send_json failed: %s", esp_err_to_name(err));
        } else {
            stats_inc(&s_stats.eth_to_dect_packets);
        }
    }
}

static bool resolve_peer(struct sockaddr_in *out)
{
    if (s_use_fixed_peer) {
        *out = s_fixed_peer;
        return true;
    }

    bool ok = false;
    if (xSemaphoreTake(s_peer_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        if (s_have_last_peer) {
            *out = s_last_peer;
            out->sin_port = htons(CONFIG_GW_UDP_REMOTE_PORT);
            ok = true;
        }
        xSemaphoreGive(s_peer_mutex);
    }

    return ok;
}

static void udp_tx_task(void *arg)
{
    (void)arg;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket tx failed: errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }

    bridge_msg_t msg;
    while (1) {
        if (xQueueReceive(s_dect_rx_q, &msg, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        struct sockaddr_in to;
        if (!resolve_peer(&to)) {
            stats_inc(&s_stats.dect_to_eth_errors);
            ESP_LOGW(TAG, "No UDP peer defined yet, dropping DECT payload");
            continue;
        }

        char json_out[CONFIG_GW_MAX_PAYLOAD_LEN * 3];
        size_t json_len = gw_json_ensure((const uint8_t *)msg.payload, msg.len, json_out, sizeof(json_out));
        if (json_len == 0) {
            stats_inc(&s_stats.dect_to_eth_errors);
            ESP_LOGW(TAG, "Failed to JSON-wrap DECT payload len=%u", (unsigned)msg.len);
            continue;
        }

        int wr = sendto(sock, json_out, json_len, 0, (struct sockaddr *)&to, sizeof(to));
        if (wr < 0) {
            stats_inc(&s_stats.dect_to_eth_errors);
            ESP_LOGW(TAG, "sendto failed: errno=%d", errno);
        } else {
            stats_inc(&s_stats.dect_to_eth_packets);
        }
    }
}

static esp_err_t load_fixed_peer(void)
{
    if (CONFIG_GW_UDP_REMOTE_IP[0] == '\0') {
        s_use_fixed_peer = false;
        return ESP_OK;
    }

    memset(&s_fixed_peer, 0, sizeof(s_fixed_peer));
    s_fixed_peer.sin_family = AF_INET;
    s_fixed_peer.sin_port = htons(CONFIG_GW_UDP_REMOTE_PORT);

    if (inet_pton(AF_INET, CONFIG_GW_UDP_REMOTE_IP, &s_fixed_peer.sin_addr) != 1) {
        ESP_LOGE(TAG, "Invalid CONFIG_GW_UDP_REMOTE_IP: %s", CONFIG_GW_UDP_REMOTE_IP);
        return ESP_ERR_INVALID_ARG;
    }

    s_use_fixed_peer = true;
    ESP_LOGI(TAG, "Using fixed UDP peer %s:%d", CONFIG_GW_UDP_REMOTE_IP, CONFIG_GW_UDP_REMOTE_PORT);
    return ESP_OK;
}

esp_err_t gw_bridge_start(void)
{
    s_dect_rx_q = xQueueCreate(16, sizeof(bridge_msg_t));
    ESP_RETURN_ON_FALSE(s_dect_rx_q != NULL, ESP_ERR_NO_MEM, TAG, "xQueueCreate failed");

    s_peer_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_peer_mutex != NULL, ESP_ERR_NO_MEM, TAG, "xSemaphoreCreateMutex failed");

    ESP_RETURN_ON_ERROR(load_fixed_peer(), TAG, "load_fixed_peer failed");

    gw_dect_set_rx_callback(dect_rx_cb, NULL);

    xTaskCreate(udp_rx_task, "gw_udp_rx", 6144, NULL, 9, NULL);
    xTaskCreate(udp_tx_task, "gw_udp_tx", 6144, NULL, 9, NULL);

    ESP_LOGI(TAG, "Gateway bridge started (ETH<->DECT)");
    return ESP_OK;
}

esp_err_t gw_bridge_get_stats(gw_bridge_stats_t *out_stats)
{
    if (out_stats == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    stats_copy(out_stats);
    return ESP_OK;
}

void gw_bridge_reset_stats(void)
{
    portENTER_CRITICAL(&s_stats_lock);
    memset(&s_stats, 0, sizeof(s_stats));
    portEXIT_CRITICAL(&s_stats_lock);
}

esp_err_t gw_bridge_set_runtime_peer(const char *ipv4, uint16_t port)
{
    if (ipv4 == NULL || ipv4[0] == '\0' || port == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    struct sockaddr_in peer = { 0 };
    peer.sin_family = AF_INET;
    peer.sin_port = htons(port);
    if (inet_pton(AF_INET, ipv4, &peer.sin_addr) != 1) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_peer_mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    s_fixed_peer = peer;
    s_use_fixed_peer = true;
    s_have_last_peer = false;
    xSemaphoreGive(s_peer_mutex);

    ESP_LOGI(TAG, "Runtime UDP peer set to %s:%u", ipv4, (unsigned)port);
    return ESP_OK;
}

esp_err_t gw_bridge_inject_dect_json(const char *json, size_t len)
{
    if (json == NULL || len == 0 || s_dect_rx_q == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    dect_rx_cb(json, len, NULL);
    return ESP_OK;
}
