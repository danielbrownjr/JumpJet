idf_component_register(
    SRCS "jj_portal.c"
    INCLUDE_DIRS "include"
    REQUIRES jj_identity jj_interlock dc_portal dc_prusa dc_wifi json esp_http_server app_update nvs_flash
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
