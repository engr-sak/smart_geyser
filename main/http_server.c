/* Developer note: Most of this code is written with the Help of ChatGPT
                    Some portions of the code modified by the human-developer
                    for fine-tuning. More human written code may appear
                    in future commits as the application develops.*/
#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <time.h>
#include <stdio.h>

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
        &timeinfo
    );

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
        "<p>ESP32 is online.</p>"
        "<p>ESP32 time: %s</p>"

        "<p>Browser time: <span id='clock'>Loading...</span></p>"

        "<script>"
            "function updateClock() {"
                "const now = new Date();"

                "document.getElementById('clock').innerHTML = "
                    "now.toLocaleTimeString();"
            "}"

            "updateClock();"
            "setInterval(updateClock, 1000);"
        "</script>"

    "</body>"
    "</html>";
    
    snprintf(
        html_resp,
        sizeof(html_resp),
        page_body,
        time_str
    );

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_resp, HTTPD_RESP_USE_STRLEN);

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
        .user_ctx = NULL
    };

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(server, &hello_uri)
    );

    ESP_LOGI(TAG, "HTTP server started");
}