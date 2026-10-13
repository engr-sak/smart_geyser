
/****************************************************
************ Add Includes/Dependencies **************
****************************************************/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "device.h"
#include "esp_timer.h"
/****************************************************
********** Define Constants For whole program **********
****************************************************/
#define DEVICE_BUTTON GPIO_NUM_21
#define DEVICE_RELAY GPIO_NUM_22
/* A status LED called Blink is used to incdicate device status*/
#define STATUS_NORMAL 0
#define STATUS_ERROR 1
#define STATUS_LED GPIO_NUM_23
#define BLINK_TIME_ON_STATUS_NORMAL 50
#define BLINK_TIME_OFF_STATUS_NORMAL 200
#define BLINK_TIME_ON_STATUS_ERROR 200
#define BLINK_TIME_OFF_STATUS_ERROR 200
#define MAX_ERROR_BLINKS 3
#define BLINK_PERIOD 1000

static const char *TAG = "geyser_controller";
/****************************************************
************** Declare Helper Functions***************
****************************************************/
int32_t status_blink(gpio_num_t led_pin, int status);
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
    int device_state = 0;      // 0 for off, 1 for on
    int btn_current_state = 1; // Assuming button is not pressed initially (active low)
    int btn_last_state = 1;    // Last state of the button
    int btn_pressed = 0;
    uint64_t btn_press_time = 0;
    uint64_t btn_release_time = 0;
    int status = STATUS_NORMAL;
    int error_blink_count = 0;
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
        vTaskDelay(1); // At lesat 1 tick delay to allow other tasks to run, if any
    }
}
/****************************************************
************** Define Helper Functions***************
****************************************************/
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
    if (status == STATUS_NORMAL)
    {
        blink_time_on = BLINK_TIME_ON_STATUS_NORMAL;
        blink_time_off = BLINK_TIME_OFF_STATUS_NORMAL;
    }
    else if (status == STATUS_ERROR)
    {
        blink_time_on = BLINK_TIME_ON_STATUS_ERROR;
        blink_time_off = BLINK_TIME_OFF_STATUS_ERROR;
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
        if ((ms_since_boot - last_blink_time) >= (BLINK_PERIOD - (2 * (blink_time_on) + blink_time_off)))
        {
            last_blink_time = ms_since_boot;
            blink_state = 0;
            blink_count++;
        }
    }
    break;
    default:
    {
        blink_state = 0;
        last_blink_time = ms_since_boot;
        break;
    }
    }
    return blink_count;
}
