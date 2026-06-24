idf_component_register(
    SRCS "mock_gw_bridge.c"
    INCLUDE_DIRS "include"
    REQUIRES lwip gw_eth gw_dect gw_json
)
