/* Developer note: Most of this code is written with the Help of ChatGPT
                    Some portions of the code modified by the human-developer
                    for fine-tuning. More human written code may appear
                    in future commits as the application develops.*/
#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <time.h>
#include <stdio.h>
// include io_manager.h to access appliance_status and wifi_status
#include "io_manager.h"
#include "wifi_manager.h"
static const char *TAG = "http_server";
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    char time_str[32];

    strftime(
        time_str,
        sizeof(time_str),
        "%Y-%m-%d %H:%M:%S",
        &timeinfo);

    char html_resp[1024];
    // char page_body1[]=
    //     "<!DOCTYPE html>"
    //     "<html>"
    //     "<head>"
    //         "<title>Smart Geyser</title>"
    //     "</head>"
    //     "<body>"
    //         "<h1>Smart Geyser Controller</h1>"
    //         "<p>ESP32 is online.</p>"
    //         "<p>Current time: %s</p>"
    //     "</body>"
    //     "</html>";
    char page_body[] =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>Smart Geyser</title>"
        "</head>"
        "<body>"
        "<h1>Smart Geyser Controller</h1>"
        "<p>Controller is online.</p>"
        "<p>Controller time: <span id='esp32_time'>Loading...</span></p>"
        "<p>Wifi status: <span id='wifi_status'>Loading...</span></p>"
        "<p>Load Status: <span id='load_status'>Loading...</span></p>"

        "<script>"

        "async function updateEsp32Time() {"

        "try {"
        "const response = await fetch('/api/status');"

        "if (!response.ok) {"
        "    throw new Error('HTTP error: ' + response.status);"
        "}"

        "const data = await response.json();"

        "document.getElementById('esp32_time').textContent = "
        "data.esp32_time;"

        "document.getElementById('wifi_status').textContent ="
        "data.wifi_status ? 'Connected' : 'Disconnected';"

        "document.getElementById('load_status').textContent ="
        "data.load_status ? 'ON' : 'OFF';"

        "} catch (error) {"

        "console.log('Error fetching ESP32 status:', error);"

        "}"

        "}"

        "updateEsp32Time();"

        "setInterval(updateEsp32Time, 5000);"

        "</script>"

        "</body>"
        "</html>";

    int bytes_written = snprintf(
        html_resp,
        sizeof(html_resp),
        page_body,
        time_str);
    if (bytes_written < 0 || bytes_written >= sizeof(html_resp))
    {
        ESP_LOGE(TAG, "ERROR: HTML status response truncated or error occurred while formatting.");
    }
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    char time_str[32];

    strftime(
        time_str,
        sizeof(time_str),
        "%Y-%m-%d %H:%M:%S",
        &timeinfo);

    char json_resp[128];
    int device_status = get_device_status();
    int wifi_status = is_wifi_connected();
    snprintf(
        json_resp,
        sizeof(json_resp),
        "{"
        "\"esp32_time\":\"%s\","
        "\"load_status\":%d,"
        "\"wifi_status\":%d"
        "}",
        time_str,
        device_status,
        wifi_status);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

void http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t hello_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = hello_get_handler,
        .user_ctx = NULL};

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &hello_uri));
    httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get_handler,
        .user_ctx = NULL};

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &status_uri));
    ESP_LOGI(TAG, "HTTP server started");
}