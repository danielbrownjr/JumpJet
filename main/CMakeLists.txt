idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES jj_identity jj_interlock jj_portal dc_evlog dc_prusa dc_wifi nvs_flash app_update
)
