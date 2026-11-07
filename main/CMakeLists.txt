idf_component_register(SRCS "main.c" "io_manager.c" "wifi_manager.c"
                    INCLUDE_DIRS "." 
                    PRIV_REQUIRES esp_wifi nvs_flash esp_driver_gpio esp_timer)