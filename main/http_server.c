/* Developer note: Most of this code is written with the Help of ChatGPT
                    Some portions of the code modified by the human-developer
                    for fine-tuning. More human written code may appear
                    in future commits as the application develops.*/
#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <time.h>
#include <stdio.h>
// includes for json parsing and string manipulation
#include "cJSON.h"
#include <string.h>
#include <stdbool.h>
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

    char page_body[] =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>Smart Geyser</title>"
        "</head>"
        "<body>"

        "<h1>Smart Geyser Controller</h1>"

        "<p>Controller time: "
        "<span id='esp32_time'>Loading...</span>"
        "</p>"

        "<p>Wifi status: "
        "<span id='wifi_status'>Loading...</span>"
        "</p>"

        "<p>Load Status: "
        "<span id='load_status'>Loading...</span>"
        "</p>"

        "<h2>Manual Control</h2>"

        "<button onclick='controlGeyser(true)'>Turn ON</button>"
        "<button onclick='controlGeyser(false)'>Turn OFF</button>"

        "<p id='command_result'></p>"

        "<script>"

        "async function updateEsp32Time() {"

        "    try {"

        "        const response = await fetch('/api/status');"

        "        if (!response.ok) {"
        "            throw new Error('HTTP error: ' + response.status);"
        "        }"

        "        const data = await response.json();"

        "        document.getElementById('esp32_time').textContent ="
        "            data.esp32_time;"

        "        document.getElementById('wifi_status').textContent ="
        "            data.wifi_status ? 'Connected' : 'Disconnected';"

        "        document.getElementById('load_status').textContent ="
        "            data.load_status ? 'ON' : 'OFF';"

        "    } catch (error) {"

        "        console.log('Error fetching ESP32 status:', error);"

        "    }"

        "}"

        "async function controlGeyser(cmd) {"

        "    try {"

        "        const response = await fetch('/api/control', {"
        "            method: 'POST',"
        "            headers: {"
        "                'Content-Type': 'application/json'"
        "            },"
        "            body: JSON.stringify({"
        "                state: cmd"
        "            })"
        "        });"

        "        if (!response.ok) {"
        "            throw new Error('HTTP error: ' + response.status);"
        "        }"

        "        const data = await response.json();"

        "        document.getElementById('command_result').textContent ="
        "            data.message;"

        "        updateEsp32Time();"

        "    } catch (error) {"

        "        document.getElementById('command_result').textContent ="
        "            'Command failed: ' + error.message;"

        "        console.log('Geyser command error:', error);"

        "    }"

        "}"

        "updateEsp32Time();"

        "setInterval(updateEsp32Time, 5000);"

        "</script>"

        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, page_body, HTTPD_RESP_USE_STRLEN);

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
static esp_err_t geyser_post_handler(httpd_req_t *req)
{
    char content[64];

    // Check request body size
    if (req->content_len == 0 ||
        req->content_len >= sizeof(content))
    {

        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Invalid request size");

        return ESP_FAIL;
    }

    // Receive the complete HTTP request body
    int received = 0;

    while (received < req->content_len)
    {

        int ret = httpd_req_recv(
            req,
            content + received,
            req->content_len - received);

        if (ret <= 0)
        {
            httpd_resp_send_err(
                req,
                HTTPD_400_BAD_REQUEST,
                "Failed to receive request");

            return ESP_FAIL;
        }

        received += ret;
    }

    content[received] = '\0';

    // Parse JSON
    cJSON *root = cJSON_Parse(content);

    if (root == NULL)
    {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Invalid JSON");

        return ESP_FAIL;
    }

    cJSON *state = cJSON_GetObjectItemCaseSensitive(root, "state");

    if (!cJSON_IsBool(state))
    {

        cJSON_Delete(root);

        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "State must be true or false");

        return ESP_FAIL;
    }

    bool turn_on = cJSON_IsTrue(state);

    cJSON_Delete(root);

    if (turn_on)
    {
        // set device to turn on
    }
    else
    {
        // set device to turn off
    }
    ESP_LOGI(TAG, "Geyser command: %s",
             turn_on ? "ON" : "OFF");

    // Send response
    const char *response = turn_on ? "{\"success\":true,\"message\":\"Geyser ON command accepted\"}" : "{\"success\":true,\"message\":\"Geyser OFF command accepted\"}";

    httpd_resp_set_type(req, "application/json");

    httpd_resp_send(
        req,
        response,
        HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}
void http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    // Serve Page
    httpd_uri_t hello_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = hello_get_handler,
        .user_ctx = NULL};

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &hello_uri));
    // Server controller status API
    httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get_handler,
        .user_ctx = NULL};

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &status_uri));
    // Server controller command API
    httpd_uri_t control_uri = {
        .uri = "/api/control",
        .method = HTTP_POST,
        .handler = geyser_post_handler,
        .user_ctx = NULL};

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &control_uri));
    ESP_LOGI(TAG, "HTTP server started");
}