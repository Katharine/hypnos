idf_component_register(SRCS "lvgl_port.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES encoder lvgl esp_lcd
                    PRIV_REQUIRES esp_lcd_gc9a01 esp_lvgl_port)
