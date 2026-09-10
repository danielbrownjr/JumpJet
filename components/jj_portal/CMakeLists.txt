idf_component_register(
    SRCS "jj_portal.c"
    INCLUDE_DIRS "include"
    REQUIRES jj_authority jj_identity jj_interlock dc_portal dc_prusa dc_wifi json esp_http_server app_update nvs_flash esp_timer esp_hw_support
    EMBED_TXTFILES "web/control.html" "web/control-client.js"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
