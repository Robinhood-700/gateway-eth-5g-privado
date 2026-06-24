idf_component_register(
    SRCS "gw_dect.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer esp_driver_gpio esp_driver_uart esp_driver_spi
)
