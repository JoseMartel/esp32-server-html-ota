#include "page_handlers.h"
#include "ota.h"
#include <esp_log.h>
#include <esp_http_server.h>
#include <string.h>

static const char *TAG = "PageHandlers";

esp_err_t index_html_get_handler(httpd_req_t *req) {
    const char *response = "<!DOCTYPE html>"
                           "<html>"
                           "<head>"
                           "<title>Firmware Over The Air</title>"
                           "<style>"
                           "body {"
                           "    font-family: Arial, sans-serif;"
                           "    text-align: center;"
                           "}"
                           "h1 {"
                           "    font-size: 24px;"
                           "}"
                           "input[type=\"file\"],"
                           "input[type=\"submit\"] {"
                           "    font-size: 16px;"
                           "    padding: 8px;"
                           "}"
                           "form {"
                           "    display: inline-block;"
                           "    margin-top: 20px;"
                           "}"
                           "</style>"
                           "</head>"
                           "<body>"
                           "<h1>Firmware Over The Air</h1>"
                           "<form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">"
                           "Select firmware: <input type=\"file\" name=\"firmware\"><br>"
                           "<input type=\"submit\" value=\"Upload\">"
                           "</form>"
                           "<p><a href=\"/about\">About</a></p>"
                           "</body>"
                           "</html>";

    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

esp_err_t about_html_get_handler(httpd_req_t *req) {
    const char *response = "<!DOCTYPE html>"
                           "<html>"
                           "<head>"
                           "<title>About</title>"
                           "</head>"
                           "<body>"
                           "<h1>About This Project</h1>"
                           "<p>This is a simple OTA demo project for ESP-IDF.</p>"
                           "<p><a href=\"/\">Home</a></p>"
                           "</body>"
                           "</html>";

    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

esp_err_t upload_post_handler(httpd_req_t *req) {
    esp_err_t err = perform_ota_update(req);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA update failed");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Firmware update complete, restarting now...");
    esp_restart();
    return ESP_OK;
}

void register_uri_handlers(httpd_handle_t server) {
    static const httpd_uri_t index_html = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_html_get_handler,
        .user_ctx  = NULL
    };

    static const httpd_uri_t about_html = {
        .uri       = "/about",
        .method    = HTTP_GET,
        .handler   = about_html_get_handler,
        .user_ctx  = NULL
    };

    static const httpd_uri_t upload = {
        .uri       = "/upload",
        .method    = HTTP_POST,
        .handler   = upload_post_handler,
        .user_ctx  = NULL
    };

    httpd_register_uri_handler(server, &index_html);
    httpd_register_uri_handler(server, &about_html);
    httpd_register_uri_handler(server, &upload);
}
