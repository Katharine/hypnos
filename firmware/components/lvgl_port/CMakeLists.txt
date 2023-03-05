idf_component_register(SRCS "lvgl_port.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES encoder lvgl esp_lcd freertos
                    PRIV_REQUIRES esp_lcd_gc9a01 esp_lvgl_port)

component_compile_options("-Wno-missing-field-initializers")
