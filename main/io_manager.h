#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

/****************************************************
******************** Constants ********************
****************************************************/
// status LED related constants
#define STATUS_NORMAL 0
#define STATUS_ERROR 1
#define STATUS_CONNECTION_LOST 2
#define BLINK_TIME_ON_STATUS_NORMAL 200
#define BLINK_TIME_OFF_STATUS_NORMAL 200
#define BLINK_TIME_ON_STATUS_ERROR 250
#define BLINK_TIME_OFF_STATUS_ERROR 200
#define BLINK_TIME_ON_STATUS_CONNECTION_LOST 100
#define BLINK_TIME_OFF_STATUS_CONNECTION_LOST 100
#define MAX_ERROR_BLINKS 3
#define BLINK_PERIOD 1000

/****************************************************
******************** Functions ********************
****************************************************/

/**
 * @brief Initialize a GPIO pin as an output device and set it to LOW.
 * 
 *        Disables the internal pull-up and pull-down resistors for the pin.
 *        Disables interrupts for the pin.
 *
 * @param[in]  device_no GPIO pin number
 *
 * @return
 *     - ESP_OK              success
 *     - ESP_FAIL            error occured
 *
 */
esp_err_t device_init(gpio_num_t device_no);

/**
 * @brief Turn on the Device
 *
 * @param[in]  device_no GPIO pin number
 *
 * @note This function Turns on the HIGH VOLTAGE device connected to the specified GPIO pin via Relay or other control mechanism.
 * @return
 *     - ESP_OK         - success
 *     - ESP_ERR_INVALID_ARG - invalid argument
 *
 */
esp_err_t device_turn_on(gpio_num_t device_no);

/**
 * @brief Turn off the Device
 *
 * @param[in]  device_no GPIO pin number
 *
 * @return
 *     - ESP_OK         - success
 *     - ESP_ERR_INVALID_ARG - invalid argument
 *
 */
esp_err_t device_turn_off(gpio_num_t device_no);

/**
 * @brief Turns On/Off the status LED based on the provided status code.
 *
 *      The function is intended to output system status through LED blink patterns.
 *      The on/off timing for each status blink is defined in header file.
 * @param[in]  led_pin GPIO pin number
 * @param[in]  status Status code
 *
 * @return
 *     - blink_count - Total Blinks happend so far, reset to zero whenever status changes
 *
 */
int32_t status_blink(gpio_num_t led_pin, int status);

#endif