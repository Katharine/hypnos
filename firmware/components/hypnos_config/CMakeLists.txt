idf_component_register(SRCS "hypnos_config.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash)

component_compile_options("-Wno-missing-field-initializers")
