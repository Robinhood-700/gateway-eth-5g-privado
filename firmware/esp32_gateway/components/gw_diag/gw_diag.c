#include "gw_diag.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gw_bridge.h"
#include "gw_dect.h"
#include "gw_eth.h"
#include "lwip/inet.h"

static const char *TAG = "gw_diag";

static uint32_t s_eth_rand_seq;
static uint32_t s_dect_rand_seq;

static void print_line(const char *line)
{
    printf("GWDBG %s\n", line);
    fflush(stdout);
}

static void print_help(void)
{
    print_line("commands:");
    print_line("  help");
    print_line("  status");
    print_line("  bridge_stats");
    print_line("  reset_stats");
    print_line("  at");
    print_line("  poe");
    print_line("  set_peer <ipv4> <port>");
    print_line("  rand_eth <count>");
    print_line("  rand_dect <count>");
    print_line("  fw_update");
    print_line("  reboot");
}

static int parse_positive_int(const char *s)
{
    if (s == NULL || s[0] == '\0') {
        return -1;
    }

    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v <= 0 || v > 100000) {
        return -1;
    }
    return (int)v;
}

static void make_random_json(const char *dir, uint32_t seq, char *out, size_t out_len)
{
    uint32_t rnd = esp_random();
    int64_t ms = esp_timer_get_time() / 1000;
    snprintf(out, out_len,
             "{\"src\":\"%s\",\"seq\":%" PRIu32 ",\"rnd\":%" PRIu32 ",\"ts_ms\":%" PRIi64 "}",
             dir, seq, rnd, ms);
}

static esp_err_t send_udp_to_gateway(const char *payload, size_t payload_len)
{
    if (!gw_eth_has_ip()) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_ip4_addr_t ip;
    ESP_RETURN_ON_ERROR(gw_eth_get_ipv4(&ip), TAG, "gw_eth_get_ipv4 failed");

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        return ESP_FAIL;
    }

    struct sockaddr_in to = {
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_GW_UDP_LISTEN_PORT),
        .sin_addr.s_addr = ip.addr,
    };

    int wr = sendto(sock, payload, payload_len, 0, (struct sockaddr *)&to, sizeof(to));
    close(sock);

    return (wr == (int)payload_len) ? ESP_OK : ESP_FAIL;
}

static void print_bridge_stats(void)
{
    gw_bridge_stats_t st = { 0 };
    if (gw_bridge_get_stats(&st) != ESP_OK) {
        print_line("bridge_stats error");
        return;
    }

    printf("GWDBG bridge eth_rx=%" PRIu64 " eth_to_dect=%" PRIu64
           " eth_err=%" PRIu64 " dect_rx=%" PRIu64
           " dect_to_eth=%" PRIu64 " dect_err=%" PRIu64 "\n",
           st.eth_rx_packets,
           st.eth_to_dect_packets,
           st.eth_to_dect_errors,
           st.dect_rx_packets,
           st.dect_to_eth_packets,
           st.dect_to_eth_errors);
    fflush(stdout);
}

static void cmd_status(void)
{
    char ipbuf[20] = "0.0.0.0";
    if (gw_eth_has_ip()) {
        esp_ip4_addr_t ip;
        if (gw_eth_get_ipv4(&ip) == ESP_OK) {
            esp_ip4addr_ntoa(&ip, ipbuf, sizeof(ipbuf));
        }
    }

    esp_err_t at = gw_dect_healthcheck();

    printf("GWDBG status eth_link=%s eth_ip=%s at=%s reset_reason=%d uptime_ms=%" PRIi64 "\n",
           gw_eth_link_up() ? "up" : "down",
           ipbuf,
           (at == ESP_OK) ? "ok" : "fail",
           (int)esp_reset_reason(),
           esp_timer_get_time() / 1000);
    fflush(stdout);

    print_bridge_stats();
}

static void cmd_poe(void)
{
#if CONFIG_GW_POE_PGOOD_GPIO < 0
    print_line("poe_pgood: not configured (set CONFIG_GW_POE_PGOOD_GPIO)");
#else
    int lvl = gpio_get_level(CONFIG_GW_POE_PGOOD_GPIO);
    bool ok = (lvl == CONFIG_GW_POE_PGOOD_ACTIVE_LEVEL);
    printf("GWDBG poe_pgood gpio=%d level=%d expected=%d result=%s\n",
           CONFIG_GW_POE_PGOOD_GPIO,
           lvl,
           CONFIG_GW_POE_PGOOD_ACTIVE_LEVEL,
           ok ? "ok" : "fail");
    fflush(stdout);
#endif
}

static void cmd_at(void)
{
    esp_err_t err = gw_dect_healthcheck();
    printf("GWDBG at %s\n", (err == ESP_OK) ? "ok" : esp_err_to_name(err));
    fflush(stdout);
}

static void cmd_set_peer(const char *ip, const char *port_str)
{
    int port = parse_positive_int(port_str);
    if (ip == NULL || port < 1 || port > 65535) {
        print_line("set_peer usage: set_peer <ipv4> <port>");
        return;
    }

    esp_err_t err = gw_bridge_set_runtime_peer(ip, (uint16_t)port);
    printf("GWDBG set_peer %s\n", (err == ESP_OK) ? "ok" : esp_err_to_name(err));
    fflush(stdout);
}

static void run_eth_random_burst(int count)
{
    char json[192];
    int ok = 0;

    for (int i = 0; i < count; ++i) {
        make_random_json("eth_in", ++s_eth_rand_seq, json, sizeof(json));
        if (send_udp_to_gateway(json, strlen(json)) == ESP_OK) {
            ok++;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    printf("GWDBG rand_eth sent=%d req=%d\n", ok, count);
    fflush(stdout);
}

static void run_dect_random_burst(int count)
{
    char json[192];
    char line[256];
    int ok = 0;

    for (int i = 0; i < count; ++i) {
        make_random_json("dect_in", ++s_dect_rand_seq, json, sizeof(json));

        if (CONFIG_GW_DECT_RX_PREFIX[0] != '\0') {
            int wr = snprintf(line, sizeof(line), "%s%s", CONFIG_GW_DECT_RX_PREFIX, json);
            if (wr <= 0 || wr >= (int)sizeof(line)) {
                continue;
            }
            if (gw_dect_test_inject_rx_line(line) == ESP_OK) {
                ok++;
            }
        } else {
            if (gw_dect_test_inject_rx_line(json) == ESP_OK) {
                ok++;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    printf("GWDBG rand_dect injected=%d req=%d\n", ok, count);
    fflush(stdout);
}

static void cmd_rand_eth(const char *count_str)
{
    int count = parse_positive_int(count_str);
    if (count < 1 || count > 1000) {
        print_line("rand_eth usage: rand_eth <count:1..1000>");
        return;
    }
    run_eth_random_burst(count);
}

static void cmd_rand_dect(const char *count_str)
{
    int count = parse_positive_int(count_str);
    if (count < 1 || count > 1000) {
        print_line("rand_dect usage: rand_dect <count:1..1000>");
        return;
    }
    run_dect_random_burst(count);
}

static void cmd_fw_update(void)
{
    print_line("USB firmware update:");
    print_line("1) source /Users/antonio/esp/esp-idf/export.sh");
    print_line("2) cd firmware/esp32_gateway");
    print_line("3) idf.py -p <USB_PORT> flash monitor");
    print_line("4) optional: use tools/usb_debug/flash_usb.sh <USB_PORT>");
}

static void execute_command(char *line)
{
    char *argv[5] = { 0 };
    int argc = 0;

    for (char *tok = strtok(line, " \t\r\n"); tok != NULL && argc < 5;
         tok = strtok(NULL, " \t\r\n")) {
        argv[argc++] = tok;
    }

    if (argc == 0) {
        return;
    }

    if (strcmp(argv[0], "help") == 0) {
        print_help();
    } else if (strcmp(argv[0], "status") == 0) {
        cmd_status();
    } else if (strcmp(argv[0], "bridge_stats") == 0) {
        print_bridge_stats();
    } else if (strcmp(argv[0], "reset_stats") == 0) {
        gw_bridge_reset_stats();
        print_line("reset_stats ok");
    } else if (strcmp(argv[0], "at") == 0) {
        cmd_at();
    } else if (strcmp(argv[0], "poe") == 0) {
        cmd_poe();
    } else if (strcmp(argv[0], "set_peer") == 0) {
        cmd_set_peer((argc > 1) ? argv[1] : NULL, (argc > 2) ? argv[2] : NULL);
    } else if (strcmp(argv[0], "rand_eth") == 0) {
        cmd_rand_eth((argc > 1) ? argv[1] : NULL);
    } else if (strcmp(argv[0], "rand_dect") == 0) {
        cmd_rand_dect((argc > 1) ? argv[1] : NULL);
    } else if (strcmp(argv[0], "fw_update") == 0) {
        cmd_fw_update();
    } else if (strcmp(argv[0], "reboot") == 0) {
        print_line("rebooting");
        esp_restart();
    } else {
        print_line("unknown command (use 'help')");
    }
}

static void usb_debug_task(void *arg)
{
    (void)arg;

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    print_line("USB debug CLI ready. type 'help'");

    char line[160];
    while (1) {
        printf("gwdbg> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        execute_command(line);
    }
}

#if CONFIG_GW_EXAMPLE_RANDOM_ENABLE
static void random_example_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "Random example traffic enabled (period=%d ms)", CONFIG_GW_EXAMPLE_RANDOM_PERIOD_MS);

    while (1) {
        run_eth_random_burst(CONFIG_GW_EXAMPLE_RANDOM_ETH_BURST);
        run_dect_random_burst(CONFIG_GW_EXAMPLE_RANDOM_DECT_BURST);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_GW_EXAMPLE_RANDOM_PERIOD_MS));
    }
}
#endif

esp_err_t gw_diag_start(void)
{
#if CONFIG_GW_POE_PGOOD_GPIO >= 0
    gpio_config_t poe_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_GW_POE_PGOOD_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&poe_cfg), TAG, "poe gpio config failed");
#endif

#if CONFIG_GW_USB_DEBUG_ENABLE
    if (xTaskCreate(usb_debug_task, "gw_usb_dbg", CONFIG_GW_USB_DEBUG_TASK_STACK, NULL,
                    CONFIG_GW_USB_DEBUG_TASK_PRIO, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
#endif

#if CONFIG_GW_EXAMPLE_RANDOM_ENABLE
    if (xTaskCreate(random_example_task, "gw_rand_ex", 4096, NULL, 6, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
#endif

    return ESP_OK;
}
