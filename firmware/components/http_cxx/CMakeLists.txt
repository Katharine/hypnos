idf_component_register(SRCS "http_cxx.cpp"
                    INCLUDE_DIRS "include"
        REQUIRES esp_http_client
        PRIV_REQUIRES mbedtls log esp-tls)

component_compile_options("-Wno-missing-field-initializers")