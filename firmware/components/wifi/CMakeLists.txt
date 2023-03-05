idf_component_register(SRCS "wifi.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES lwip esp_wifi esp_event esp_common log esp_hw_support nvs_flash)

component_compile_options("-Wno-missing-field-initializers")
