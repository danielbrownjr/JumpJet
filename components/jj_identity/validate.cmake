function(jj_validate_product_id product_id)
    if("${product_id}" STREQUAL "")
        message(FATAL_ERROR "JJ_IDENTITY_PRODUCT_ID must be defined and non-empty")
    endif()
    string(LENGTH "${product_id}" product_id_length)
    # esp_app_desc_t.project_name is 32 bytes including its terminating NUL.
    if(product_id_length GREATER 31)
        message(FATAL_ERROR
            "JJ_IDENTITY_PRODUCT_ID must fit esp_app_desc_t.project_name (31 bytes maximum)")
    endif()
endfunction()
