
#include "io_manager.h"
#include <stdio.h>
static const char *TAG = "IO_CTRL";
esp_err_t device_init(gpio_num_t DEVICE_NO)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << DEVICE_NO);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&io_conf) == ESP_OK)
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
    ESP_LOGI(TAG, "Turning on the device %d", DEVICE_NO);
    return gpio_set_level(DEVICE_NO, 1);
}

esp_err_t device_turn_off(gpio_num_t DEVICE_NO)
{
    ESP_LOGI(TAG, "Turning off the device %d ", DEVICE_NO);
    return gpio_set_level(DEVICE_NO, 0);
}

int32_t status_blink(gpio_num_t led_pin, int status)
{
    // struct timeval tv;
    // gettimeofday(&tv,NULL);
    uint64_t ms_since_boot = esp_timer_get_time() / 1000;
    static int blink_state = 0;
    static uint64_t last_blink_time = 0;
    static int last_blink_status = STATUS_NORMAL;
    uint64_t blink_time_on = 0;
    uint64_t blink_time_off = 0;
    static int blink_count = 0;
    int blink_rate = 1; // How many blinks per period time, default is 1
    switch (status)
    {
    case STATUS_NORMAL:
    {
        blink_time_on = BLINK_TIME_ON_STATUS_NORMAL;
        blink_time_off = BLINK_TIME_OFF_STATUS_NORMAL;
        blink_rate = 1;
        break;
    }
    case STATUS_ERROR:
    {
        blink_time_on = BLINK_TIME_ON_STATUS_ERROR;
        blink_time_off = BLINK_TIME_OFF_STATUS_ERROR;
        blink_rate = 2;
        break;
    }
    case STATUS_CONNECTION_LOST:
    {
        blink_time_on = BLINK_TIME_ON_STATUS_CONNECTION_LOST;
        blink_time_off = BLINK_TIME_OFF_STATUS_CONNECTION_LOST;
        blink_rate = 2;
        break;
    }
    default:
    {
        blink_time_on = BLINK_TIME_ON_STATUS_NORMAL;
        blink_time_off = BLINK_TIME_OFF_STATUS_NORMAL;
        blink_rate = 1;
        break;
    }
    }
    // Reset Blink state and count if status has changed
    if (last_blink_status != status)
    {
        blink_state = 0;
        last_blink_time = 0;
        blink_count = 0;
    }
    last_blink_status = status;
    switch (blink_state)
    {
    case 0:
    {
        // turn on the LED for first Blink
        if ((ms_since_boot - last_blink_time) >= blink_time_off)
        {
            ESP_ERROR_CHECK(gpio_set_level(led_pin, 1));
            last_blink_time = ms_since_boot;
            blink_state = 1;
        }
        break;
    }
    case 1:
    {
        // turn off the LED for first Blink
        if ((ms_since_boot - last_blink_time) >= blink_time_on)
        {
            ESP_ERROR_CHECK(gpio_set_level(led_pin, 0));
            last_blink_time = ms_since_boot;
            if (blink_rate == 1)
                blink_state = 4; // If only one blink per period, go to wait state
            else
                blink_state = 2;
        }
        break;
    }
    // second blink
    case 2:
    {
        // turn on the LED for second Blink
        if ((ms_since_boot - last_blink_time) >= blink_time_off)
        {
            ESP_ERROR_CHECK(gpio_set_level(led_pin, 1));
            last_blink_time = ms_since_boot;
            blink_state = 3;
        }
        break;
    }
    case 3:
    {
        // turn off the LED for second Blink
        if ((ms_since_boot - last_blink_time) >= blink_time_on)
        {
            ESP_ERROR_CHECK(gpio_set_level(led_pin, 0));
            last_blink_time = ms_since_boot;
            blink_state = 4;
        }
        break;
    }
    case 4:
    {
        // wait period before next blink sequence
        int64_t net_period = (BLINK_PERIOD - ((blink_rate * blink_time_on) + blink_time_off));
        if ((ms_since_boot - last_blink_time) >= net_period)
        {
            last_blink_time = ms_since_boot;
            blink_state = 0;
            blink_count++;
        }
        break;
    }
    default:
    {
        blink_state = 0;
        last_blink_time = ms_since_boot;
        break;
    }
    }
    return blink_count;
}
