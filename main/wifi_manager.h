#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
/****************************************************
********** Define Configurations **********
****************************************************/

/*
Set the Wifi SSID and Password in the following defines.
*/
#define EXAMPLE_ESP_WIFI_SSID "ESP32_TEST"
#define EXAMPLE_ESP_WIFI_PASS "12345678"
#define EXAMPLE_ESP_MAXIMUM_RETRY 5
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

/****************************************************
******************** Functions ********************
****************************************************/

/**
 * @brief Initialize the WiFi in station mode
 * 
 *        Copy from ESP32 Station Example on GitHub
 *
 * @return
 *     none
 *
 */
void wifi_init_sta(void);

/**
 * @brief Checks WiFi connenction status
 * 
 *        Connection status is not updated abruptly, there are internal reconnect attempts before declaring the Disconnection State.
 *
 * @return
 *     - connection_status - Connected(1) or Not Connectect(0)
 *
 */
int is_wifi_connected(void);

/**
 * @brief Restarts reconnection attempts
 *
 *        Manages reconnect start time internally. Repeated calls do not take effect untile reconnect attempt expiry time reached.
 * @return
 *     - s_retry_num - No of reconnect attempts done so far.
 *
 */
int try_wifi_reconnect(void);

#endif