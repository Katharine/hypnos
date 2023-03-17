idf_component_register(SRCS "backlight.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_adc
                    PRIV_REQUIRES log statics lvgl_port)
