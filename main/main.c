
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

static const char *TAG = "MAIN_CTRL";
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

    /* For now Define Hard Code Turn On/Off times*/
    struct tm schedule_time_on = {
        .tm_year = 2026 - 1900,
        .tm_mon = 11 - 1,
        .tm_mday = 7,
        .tm_hour = 23,
        .tm_min = 54,
        .tm_sec = 15,
        .tm_isdst = -1};

    struct tm schedule_time_off = {
        .tm_year = 2026 - 1900,
        .tm_mon = 11 - 1,
        .tm_mday = 7,
        .tm_hour = 23,
        .tm_min = 57,
        .tm_sec = 15,
        .tm_isdst = -1};
    setenv("TZ", "PKT-5", 1);
    tzset();
    time_t turn_on_time = mktime(&schedule_time_on);
    time_t turn_off_time = mktime(&schedule_time_off);
    time_t current_time;
    int schedule_started = 0;   // Flag to indicate if the schedule has started;
    int schedule_completed = 0; // Flag to indicate if the schedule has completed;

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
            if (ESP_OK == device_turn_on(DEVICE_RELAY))
            {
                device_state = 1;
            }
            btn_pressed = 0; // Reset button pressed state after handling
        }
        if (btn_pressed == 1 && device_state == 1)
        {
            if (ESP_OK == device_turn_off(DEVICE_RELAY))
            {
                device_state = 0;
            }
            btn_pressed = 0; // Reset button pressed state after handling
        }
        if (schedule_started == 0)
        {
            // Check if it's time to turn on the device based on the schedule
            time(&current_time);
            if (current_time >= turn_on_time && device_state == 0)
            {
                ESP_LOGI(TAG, "Scheduled turn on time reached");
                if (ESP_OK == device_turn_on(DEVICE_RELAY))
                {
                    schedule_started = 1;
                    schedule_completed = 0;
                    device_state = 1;
                }
            }
        }
        if (schedule_completed == 0)
        {
            // Check if it's time to turn off the device based on the schedule
            time(&current_time);
            if (current_time >= turn_off_time && device_state == 1)
            {
                ESP_LOGI(TAG, "Scheduled turn off time reached");
                if (ESP_OK == device_turn_off(DEVICE_RELAY))
                {
                    device_state = 0;
                    schedule_completed = 1;
                }
            }
        }
        uint64_t ms_since_boot = esp_timer_get_time() / 1000;
        // detect wifi disconnect but let the Error State take precedence
        if (is_wifi_connected() == 0 && status != STATUS_ERROR)
        {
            if (ms_since_boot - last_debug_msg_time >= 5000) // Log every 5 seconds
            {
                // ESP_LOGI(TAG, "WiFi connection lost.");
                last_debug_msg_time = ms_since_boot;
                try_wifi_reconnect();
            }
            status = STATUS_CONNECTION_LOST;
        }
        else if (status != STATUS_ERROR)
        {
            if (ms_since_boot - last_debug_msg_time >= 5000) // Log every 5 seconds
            {
                // ESP_LOGI(TAG, "WiFi connected.");
                time(&current_time);
                char strftime_buf_current[64];
                char strftime_buf_on[64];

                // Convert time_t to tm struct safely
                struct tm timeinfo_current;
                localtime_r(&current_time, &timeinfo_current);

                // Format the time string (e.g., "YYYY-MM-DD HH:MM:SS")
                strftime(strftime_buf_current, sizeof(strftime_buf_current), "%Y-%m-%d %H:%M:%S", &timeinfo_current);
                strftime(strftime_buf_on, sizeof(strftime_buf_on), "%Y-%m-%d %H:%M:%S", &schedule_time_on);

                ESP_LOGI(TAG, "Current time: %s,On TTime: %s", strftime_buf_current, strftime_buf_on);
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
