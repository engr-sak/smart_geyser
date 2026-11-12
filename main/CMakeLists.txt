idf_component_register(
    SRCS
        "main.c"
        "io_manager.c"
        "wifi_manager.c"
        "http_server.c"
        "filesystem.c"

    INCLUDE_DIRS "."

    PRIV_REQUIRES
        esp_wifi
        nvs_flash
        esp_driver_gpio
        esp_timer
        esp_netif
        esp_http_server
        spiffs
)
spiffs_create_partition_image(
    web_storage
    "../web_ui"
    FLASH_IN_PROJECT
)