#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
esp_err_t device_init(gpio_num_t DEVICE_NO);
esp_err_t device_turn_on(gpio_num_t DEVICE_NO);
esp_err_t device_turn_off(gpio_num_t DEVICE_NO);