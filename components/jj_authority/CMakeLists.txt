idf_component_register(
    SRCS "jj_authority.c"
    INCLUDE_DIRS "include"
    REQUIRES jj_interlock
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
