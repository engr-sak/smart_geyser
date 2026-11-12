#include "filesystem.h"

#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "file_system";

esp_err_t filesystem_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "web_storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0;
    size_t used = 0;

    ret = esp_spiffs_info("web_storage", &total, &used);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS total: %u bytes, used: %u bytes",
                 (unsigned)total, (unsigned)used);
    }

    return ESP_OK;
}