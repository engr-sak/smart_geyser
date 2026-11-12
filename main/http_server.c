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
// filesystem includes
// #include "filesystem.h"
#include <sys/stat.h>
static const char *TAG = "http_server";
// Handler for serving the HTML page
static esp_err_t send_file(
    httpd_req_t *req,
    const char *filepath,
    const char *content_type)
{
    FILE *file = fopen(filepath, "r");

    if (file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);

        httpd_resp_send_err(
            req,
            HTTPD_404_NOT_FOUND,
            "File not found");

        return ESP_FAIL;
    }

    httpd_resp_set_type(req, content_type);

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK)
        {
            fclose(file);
            return ESP_FAIL;
        }
    }

    fclose(file);

    // Signal that the HTTP response is complete
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}
static esp_err_t display_ui_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving index.html");
    return send_file(
        req,
        "/spiffs/index.html",
        "text/html");
}
static esp_err_t javascript_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving scripts.js");
    return send_file(
        req,
        "/spiffs/scripts.js",
        "application/javascript");
}

static esp_err_t css_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving style.css");
    return send_file(
        req,
        "/spiffs/style.css",
        "text/css");
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
    httpd_uri_t display_ui_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = display_ui_handler,
        .user_ctx = NULL};
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &display_ui_uri));

    // Serve JavaScript
    httpd_uri_t javascript_uri = {
        .uri = "/scripts.js",
        .method = HTTP_GET,
        .handler = javascript_handler,
        .user_ctx = NULL};
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &javascript_uri));

    // Serve CSS
    httpd_uri_t css_uri = {
        .uri = "/style.css",
        .method = HTTP_GET,
        .handler = css_handler,
        .user_ctx = NULL};
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &css_uri));

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