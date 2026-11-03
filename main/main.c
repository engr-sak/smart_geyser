
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
#include "device.h"
#include "esp_timer.h"
// Wifi includes
#include <string.h>
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/****************************************************
********** Define Constants For whole program **********
****************************************************/
#define DEVICE_BUTTON GPIO_NUM_21
#define DEVICE_RELAY GPIO_NUM_22
/* A status LED called Blink is used to incdicate device status*/
#define STATUS_NORMAL 0
#define STATUS_ERROR 1
#define STATUS_CONNECTION_LOST 2
#define STATUS_LED GPIO_NUM_23
#define BLINK_TIME_ON_STATUS_NORMAL 200
#define BLINK_TIME_OFF_STATUS_NORMAL 200
#define BLINK_TIME_ON_STATUS_ERROR 250
#define BLINK_TIME_OFF_STATUS_ERROR 200
#define BLINK_TIME_ON_STATUS_CONNECTION_LOST 100
#define BLINK_TIME_OFF_STATUS_CONNECTION_LOST 100
#define MAX_ERROR_BLINKS 3
#define BLINK_PERIOD 1000

static const char *TAG = "geyser_controller";
/****************************************************
************** Declare Helper Functions***************
****************************************************/
int32_t status_blink(gpio_num_t led_pin, int status);
void wifi_init_sta(void);
int is_wifi_connected(void);
int try_wifi_reconnect(void);
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

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {
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
        uint64_t last_debug_msg_time=0;
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
            if(ms_since_boot- last_debug_msg_time >= 1000) // Log every 5 seconds
            {
                ESP_LOGI(TAG, "WiFi connection lost.");
                last_debug_msg_time = ms_since_boot;
                try_wifi_reconnect();
            }
            status = STATUS_CONNECTION_LOST;
        }
        else if(status!=STATUS_ERROR){
            if(ms_since_boot- last_debug_msg_time >= 5000) // Log every 5 seconds
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
        blink_rate=1;
        break;
    }
    case STATUS_ERROR:
    {
        blink_time_on = BLINK_TIME_ON_STATUS_ERROR;
        blink_time_off = BLINK_TIME_OFF_STATUS_ERROR;
        blink_rate=2;
        break;
    }
    case STATUS_CONNECTION_LOST:
    {
        blink_time_on = BLINK_TIME_ON_STATUS_CONNECTION_LOST;
        blink_time_off = BLINK_TIME_OFF_STATUS_CONNECTION_LOST;
        blink_rate=2;
        break;
    }
    default:
    {
        blink_time_on = BLINK_TIME_ON_STATUS_NORMAL;
        blink_time_off = BLINK_TIME_OFF_STATUS_NORMAL;
        blink_rate=1;
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
            if(blink_rate==1)
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

/**** Wifi API starts below */
/* WiFi station Example Code Copy from github

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
/*
Set the Wifi SSID and Password in the following defines.
*/
#define EXAMPLE_ESP_WIFI_SSID      "ESP32_TEST"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5
#define CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK 1
#define CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HUNT_AND_PECK 1

#if CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG_WIFI = "wifi station";

static int s_retry_num = 0;


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_WIFI, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
        ESP_LOGI(TAG_WIFI, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if auth mode threshold equals WIFI_AUTH_OPEN
             * and password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
#ifdef CONFIG_ESP_WIFI_WPA3_COMPATIBLE_SUPPORT
            .disable_wpa3_compatible_mode = 0,
#endif
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");

    // /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
    //  * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    // EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
    //                                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
    //                                        pdFALSE,
    //                                        pdFALSE,
    //                                        portMAX_DELAY);

    // /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    //  * happened. */
    // if (bits & WIFI_CONNECTED_BIT)
    // {
    //     ESP_LOGI(TAG_WIFI, "connected to ap SSID:%s password:%s",
    //              EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    // }
    // else if (bits & WIFI_FAIL_BIT)
    // {
    //     ESP_LOGI(TAG_WIFI, "Failed to connect to SSID:%s, password:%s",
    //              EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    // }
    // else
    // {
    //     ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");
    // }
}
/* Check for wifi connection status */
int is_wifi_connected(void)
{
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    int connection_status = ((bits & WIFI_CONNECTED_BIT) != 0) && ((bits & WIFI_FAIL_BIT) == 0);
    return connection_status;
}
uint64_t last_reconnect_attempt_time = 0;
int try_wifi_reconnect()
{
    uint64_t ms_since_boot = esp_timer_get_time() / 1000;
    if (s_retry_num >= EXAMPLE_ESP_MAXIMUM_RETRY && ((ms_since_boot - last_reconnect_attempt_time) >= 20000)) // Retry every 20 seconds
    {
        s_retry_num = 0;
        last_reconnect_attempt_time = ms_since_boot;
        // esp_wifi_start();
        if (esp_wifi_connect() == ESP_OK)
        {
            ESP_LOGI(TAG_WIFI, "[ReConnect]:Initiating WiFi reconnect...");
        }
        else
        {
            ESP_LOGE(TAG_WIFI, "[ReConnect]:WiFi reconnect initiation failed.");
        }
    }
    return s_retry_num;
}