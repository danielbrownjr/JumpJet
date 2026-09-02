idf_component_register(SRCS "jj_interlock.c" INCLUDE_DIRS "include" REQUIRES freertos)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
