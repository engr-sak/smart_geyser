
#include "device.h"
#include <stdio.h>
static const char *TAG = "DEVICE";
esp_err_t device_init(gpio_num_t DEVICE_NO)
{
     gpio_config_t io_conf = {};
     io_conf.pin_bit_mask = (1ULL << DEVICE_NO);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        if(gpio_config(&io_conf)==ESP_OK)
        {
            gpio_set_level(DEVICE_NO, 0);
            printf("GPIO %d configured as output and set to LOW\n", DEVICE_NO);
        }
        else
        {
                ESP_LOGE(TAG, "GPIO configuration failed for GPIO %d", DEVICE_NO);
                return ESP_FAIL;
        }
        return ESP_OK;
}
esp_err_t device_turn_on(gpio_num_t DEVICE_NO)
{
    return gpio_set_level(DEVICE_NO, 1);
}

esp_err_t device_turn_off(gpio_num_t DEVICE_NO)
{
    return gpio_set_level(DEVICE_NO, 0);
}