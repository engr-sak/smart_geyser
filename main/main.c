
/****************************************************
************ Add Includes/Dependencies **************
****************************************************/
// GPIO and general includes
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
// Gpio control apis
#include "io_manager.h"
// Wifi includes
#include "wifi_manager.h"
/****************************************************
********** Define Constants For whole program **********
****************************************************/
// Define GPIO Pin Map
#define DEVICE_BUTTON GPIO_NUM_21
#define DEVICE_RELAY GPIO_NUM_22
/* A status LED called Blink is used to incdicate device status*/
#define STATUS_LED GPIO_NUM_23 

static const char *TAG = "geyser_controller";
/****************************************************
************** Declare Helper Functions***************
****************************************************/

/****************************************************
************** App Main Starts Here ***************
****************************************************/
void app_main(void)
{
    gpio_config_t io_conf_status_led = {
        .pin_bit_mask = (1ULL << STATUS_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config_t io_conf_control_btn = {
        .pin_bit_mask = (1ULL << DEVICE_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf_status_led));
    ESP_ERROR_CHECK(gpio_config(&io_conf_control_btn));
    ESP_ERROR_CHECK(device_init(DEVICE_RELAY));
    ESP_LOGI(TAG, "Status LED initialized on GPIO %d", STATUS_LED);
    /* Start up wifi */

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL)
    {
        /* If you only want to open more logs in the wifi module, you need to make the max level greater than the default level,
         * and call esp_log_level_set() before esp_wifi_init() to improve the log level of the wifi module. */
        esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL);
    }

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    ESP_LOGI(TAG, "WiFi initialization complete.");
    int device_state = 0;      // 0 for off, 1 for on
    int btn_current_state = 1; // Assuming button is not pressed initially (active low)
    int btn_last_state = 1;    // Last state of the button
    int btn_pressed = 0;
    uint64_t btn_press_time = 0;
    uint64_t btn_release_time = 0;
    int status = STATUS_NORMAL;
    int error_blink_count = 0;
    uint64_t last_debug_msg_time = 0;
    while (1)
    {
        // Blink the status LED
        int32_t blink_count = status_blink(STATUS_LED, status);
        if (status == STATUS_ERROR)
        {
            error_blink_count = blink_count;
            if (error_blink_count >= MAX_ERROR_BLINKS)
            {
                ESP_LOGI(TAG, "Max error blinks reached, resetting status to normal.");
                status = STATUS_NORMAL;
                error_blink_count = 0;
            }
        }
        btn_current_state = gpio_get_level(DEVICE_BUTTON);
        if (btn_current_state == 0 && btn_last_state == 1)
        {
            // falling edge detected, button pressed
            btn_pressed = 0;
            btn_last_state = 0;
            btn_press_time = esp_timer_get_time() / 1000; // Record the time when button is pressed
        }
        else if (btn_current_state == 1 && btn_last_state == 0)
        {
            // rising edge detected, button released
            btn_release_time = esp_timer_get_time() / 1000; // Record the time when button is released
            btn_last_state = 1;
            int64_t duration = btn_release_time - btn_press_time; // Calculate the duration of the button press
            if ((duration) >= 5000)                               // Check if button was pressed for at least 5 seconds
            {
                btn_pressed = 1; // Set the button pressed flag
            }
            else
            {
                ESP_LOGI(TAG, "Button Press Duration not met, command ignored, measured: %lld ms", duration);
                status = STATUS_ERROR;
            }
        }

        if (btn_pressed == 1 && device_state == 0)
        {
            ESP_LOGI(TAG, "Turning on the device.");
            if (ESP_OK == device_turn_on(DEVICE_RELAY))
            {
                device_state = 1;
            }
            btn_pressed = 0; // Reset button pressed state after handling
        }
        if (btn_pressed == 1 && device_state == 1)
        {
            ESP_LOGI(TAG, "Turning off the device.");
            if (ESP_OK == device_turn_off(DEVICE_RELAY))
            {
                device_state = 0;
            }
            btn_pressed = 0; // Reset button pressed state after handling
        }
        uint64_t ms_since_boot = esp_timer_get_time() / 1000;
        // detect wifi disconnect but let the Error State take precedence
        if (is_wifi_connected() == 0 && status != STATUS_ERROR)
        {
            if (ms_since_boot - last_debug_msg_time >= 1000) // Log every 5 seconds
            {
                ESP_LOGI(TAG, "WiFi connection lost.");
                last_debug_msg_time = ms_since_boot;
                try_wifi_reconnect();
            }
            status = STATUS_CONNECTION_LOST;
        }
        else if (status != STATUS_ERROR)
        {
            if (ms_since_boot - last_debug_msg_time >= 5000) // Log every 5 seconds
            {
                ESP_LOGI(TAG, "WiFi connected.");
                last_debug_msg_time = ms_since_boot;
            }
            status = STATUS_NORMAL;
        }
        vTaskDelay(2); // At lesat 1 tick delay to allow other tasks to run, if any
    }
}
/****************************************************
************** Define Helper Functions***************
****************************************************/
