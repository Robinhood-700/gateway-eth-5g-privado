idf_component_register(
    SRCS "gw_watchdog.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_system esp_timer gw_eth gw_dect
)
