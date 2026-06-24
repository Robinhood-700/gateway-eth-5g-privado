idf_component_register(
    SRCS "mock_gw_diag.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_system esp_timer esp_netif lwip esp_driver_gpio gw_eth gw_dect gw_bridge
)
