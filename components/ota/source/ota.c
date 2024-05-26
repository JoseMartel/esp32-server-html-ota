#include "ota.h"
#include <esp_log.h>
#include <esp_ota_ops.h>

static const char *TAG = "OTA";

esp_err_t perform_ota_update(httpd_req_t *req) {
    char buf[1024];
    int received = 0;
    esp_ota_handle_t ota_handle;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
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
                    ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                    esp_ota_end(ota_handle);
                    return ESP_FAIL;
                }
            }
        } else {
            err = esp_ota_write(ota_handle, buf, received);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                esp_ota_end(ota_handle);
                return ESP_FAIL;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (received < 0 && received != HTTPD_SOCK_ERR_TIMEOUT) {
        ESP_LOGE(TAG, "File reception failed!");
        esp_ota_end(ota_handle);
        return ESP_FAIL;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        return ESP_FAIL;
    }

    return ESP_OK;
}
