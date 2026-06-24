idf_component_register(
    SRCS "gw_bridge.c"
    INCLUDE_DIRS "include"
    REQUIRES lwip gw_eth gw_dect gw_json
)
