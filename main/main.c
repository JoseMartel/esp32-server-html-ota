#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_ota_ops.h>
#include <esp_http_server.h>
#include "protocol_examples_common.h"

#define MAX_HTTP_URI_LEN 512
#define MAX_HTTP_HDR_LEN 1024

void printHex(const char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", (unsigned char)buf[i]);
    }
    printf("\n");
}

static esp_err_t index_html_get_handler(httpd_req_t *req) {
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
                           "</body>"
                           "</html>";

    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

static esp_err_t upload_post_handler(httpd_req_t *req) {
    char buf[1024];
    int received = 0;
    esp_ota_handle_t ota_handle;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE("OTA", "esp_ota_begin failed (%s)", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
        return ESP_FAIL;
    }

    bool is_header_parsed = false;
    char *content_start = NULL;
    int content_length = 0;

    while ((received = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
        if (!is_header_parsed) {
            content_start = strstr(buf, "\r\n\r\n");
            if (content_start) {
                content_start += 4; // Move past the "\r\n\r\n"
                content_length = received - (content_start - buf);
                is_header_parsed = true;

                // Write the remaining content after the header
                err = esp_ota_write(ota_handle, content_start, content_length);
                if (err != ESP_OK) {
                    ESP_LOGE("OTA", "esp_ota_write failed (%s)", esp_err_to_name(err));
                    esp_ota_end(ota_handle);
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
                    return ESP_FAIL;
                }
            }
        } else {
            // Print content for debugging
            // printf("Received data: %.*s\n", 512, buf);
            // printf("Received data (Hexadecimal): ");
            // printHex(buf, 512);

            err = esp_ota_write(ota_handle, buf, received);
            if (err != ESP_OK) {
                ESP_LOGE("OTA", "esp_ota_write failed (%s)", esp_err_to_name(err));
                esp_ota_end(ota_handle);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
                return ESP_FAIL;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (received < 0 && received != HTTPD_SOCK_ERR_TIMEOUT) {
        ESP_LOGE("OTA", "File reception failed!");
        esp_ota_end(ota_handle);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File reception failed");
        return ESP_FAIL;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE("OTA", "esp_ota_end failed (%s)!", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA end failed");
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE("OTA", "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA set boot partition failed");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Firmware update complete, restarting now...");
    esp_restart();
    return ESP_OK;
}

static const httpd_uri_t index_html = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_html_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t upload = {
    .uri       = "/upload",
    .method    = HTTP_POST,
    .handler   = upload_post_handler,
    .user_ctx  = NULL
};

void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = MAX_HTTP_URI_LEN;
    config.max_resp_headers = MAX_HTTP_HDR_LEN;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &index_html);
        httpd_register_uri_handler(server, &upload);
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    start_webserver();

    while (true)
    {   
        printf("\n OTA Failed - Old version\r");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}