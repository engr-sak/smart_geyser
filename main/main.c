
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
/****************************************************
********** Define Constants For whole program **********
****************************************************/
#define DEVICE_BUTTON GPIO_NUM_21
#define DEVICE_RELAY GPIO_NUM_22
#define STATUS_LED GPIO_NUM_23
#define BLINK_TIME_ON_STATUS_NORMAL 50
#define BLINK_TIME_OFF_STATUS_NORMAL 200
#define BLINK_PERIOD 1000

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
    int device_state = 0;      // 0 for off, 1 for on
    int btn_current_state = 1; // Assuming button is not pressed initially (active low)
    int btn_last_state = 1;    // Last state of the button
    int btn_pressed = 0;
    while (1)
    {
        // first blink
        ESP_ERROR_CHECK(gpio_set_level(STATUS_LED, 1));
        vTaskDelay(pdMS_TO_TICKS(BLINK_TIME_ON_STATUS_NORMAL));

        ESP_ERROR_CHECK(gpio_set_level(STATUS_LED, 0));
        vTaskDelay(pdMS_TO_TICKS(BLINK_TIME_OFF_STATUS_NORMAL));
        // second blink
        ESP_ERROR_CHECK(gpio_set_level(STATUS_LED, 1));
        vTaskDelay(pdMS_TO_TICKS(BLINK_TIME_ON_STATUS_NORMAL));

        ESP_ERROR_CHECK(gpio_set_level(STATUS_LED, 0));
        vTaskDelay(pdMS_TO_TICKS(BLINK_TIME_OFF_STATUS_NORMAL));
        // blink period delay
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD - (2 * (BLINK_TIME_ON_STATUS_NORMAL + BLINK_TIME_OFF_STATUS_NORMAL))));
        btn_current_state = gpio_get_level(DEVICE_BUTTON);
        if (btn_current_state == 0 && btn_last_state == 1)
        {
            // falling edge detected, button pressed
            btn_pressed = 0;
            btn_last_state = 0;
        }
        else if (btn_current_state == 1 && btn_last_state == 0)
        {
            // rising edge detected, button released
            btn_pressed = 1;
            btn_last_state = 1;
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
    }
}