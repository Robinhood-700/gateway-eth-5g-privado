idf_component_register(
    SRCS "gw_eth.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_eth esp_netif esp_event esp_driver_gpio esp_driver_spi espressif__w5500
)
