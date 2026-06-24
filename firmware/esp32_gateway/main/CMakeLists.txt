idf_component_register(
    SRCS "mock_app_main.c"
    INCLUDE_DIRS "."
    REQUIRES gw_eth gw_dect gw_bridge gw_watchdog gw_diag nvs_flash
)
