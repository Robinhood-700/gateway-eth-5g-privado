#include "gw_diag.h"

#include <stdio.h>

static void mock_print_line(const char *line)
{
    printf("[MOCK] %s\n", line);
}

static void mock_print_help(void)
{
    mock_print_line("help");
    mock_print_line("status");
    mock_print_line("bridge_stats");
    mock_print_line("reset_stats");
}

static void mock_execute_command(const char *cmd)
{
    if (cmd == NULL) {
        return;
    }

    printf("[MOCK] Executing command: %s\n", cmd);
}

esp_err_t mock_gw_diag_start(void)
{
    mock_print_line("Diagnostic subsystem started");
    return ESP_OK;
}
