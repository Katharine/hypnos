idf_component_register(SRCS "backlight.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES lvgl_port esp_adc
                    PRIV_REQUIRES log)
