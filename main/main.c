
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#define STATUS_LED GPIO_NUM_23
#define BLINK_TIME_ON_STATUS_NORMAL 50
#define BLINK_TIME_OFF_STATUS_NORMAL 200
#define BLINK_PERIOD 1000

static const char *TAG = "geyser_controller";

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STATUS_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_LOGI(TAG, "LED blink test started on GPIO %d", STATUS_LED);

    while (1)
    {
        // first blink
        ESP_ERROR_CHECK(gpio_set_level(STATUS_LED, 1));
        ESP_LOGI(TAG, "LED ON");
        vTaskDelay(pdMS_TO_TICKS(BLINK_TIME_ON_STATUS_NORMAL));

        ESP_ERROR_CHECK(gpio_set_level(STATUS_LED, 0));
        ESP_LOGI(TAG, "LED OFF");
        vTaskDelay(pdMS_TO_TICKS(BLINK_TIME_OFF_STATUS_NORMAL));
        // second blink
        ESP_ERROR_CHECK(gpio_set_level(STATUS_LED, 1));
        ESP_LOGI(TAG, "LED ON");
        vTaskDelay(pdMS_TO_TICKS(BLINK_TIME_ON_STATUS_NORMAL));

        ESP_ERROR_CHECK(gpio_set_level(STATUS_LED, 0));
        ESP_LOGI(TAG, "LED OFF");
        vTaskDelay(pdMS_TO_TICKS(BLINK_TIME_OFF_STATUS_NORMAL));
        // blink period delay
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD - (2 * (BLINK_TIME_ON_STATUS_NORMAL + BLINK_TIME_OFF_STATUS_NORMAL))));
    }
}
