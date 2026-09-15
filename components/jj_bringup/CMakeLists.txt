idf_component_register(
    SRCS "jj_bringup.c" "jj_bringup_json.c"
    INCLUDE_DIRS "include"
    REQUIRES jj_board json dc_wifi esp_app_format esp_hw_support spi_flash heap esp_netif esp_system esp_timer esp_wifi esp_psram
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
