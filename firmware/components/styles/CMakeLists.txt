idf_component_register(SRCS "styles.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES lvgl
                    PRIV_REQUIRES lvgl_port log wifi_widget)

component_compile_options("-Wno-missing-field-initializers" "-Wno-deprecated-enum-enum-conversion")
