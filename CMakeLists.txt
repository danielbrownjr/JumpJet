cmake_minimum_required(VERSION 3.16)
include(${CMAKE_CURRENT_SOURCE_DIR}/components/jj_identity/identity.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/components/jj_identity/validate.cmake)
jj_validate_product_id("${JJ_IDENTITY_PRODUCT_ID}")
execute_process(COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE JJ_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT JJ_VERSION)
    set(JJ_VERSION "development")
endif()
set(PROJECT_VER "${JJ_VERSION}")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${JJ_IDENTITY_PRODUCT_ID})
