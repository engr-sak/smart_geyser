idf_component_register(SRCS "main.c" "device.c"
                    INCLUDE_DIRS "." 
                    PRIV_REQUIRES esp_wifi nvs_flash esp_driver_gpio esp_timer)