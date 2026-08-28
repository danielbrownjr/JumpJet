set(jj_identity_include_dir "${CMAKE_CURRENT_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${jj_identity_include_dir}")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/include/jj_identity.h.in"
    "${jj_identity_include_dir}/jj_identity.h"
    @ONLY
)

idf_component_register(INCLUDE_DIRS "${jj_identity_include_dir}")
