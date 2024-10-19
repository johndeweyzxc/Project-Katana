idf_component_register(
    SRCS "dns_server.c" "cmd_parser.c" "deauther.c" "dns_server.c" "dhcp_server.c" "injector.c" "web_server.c" "wifi_ctl.c" "main.c" "utils.c" "frame_eapol_validator.c" "frame_output.c" "frame_parser.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_event esp_netif esp_http_server json esp_wifi
)

target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)