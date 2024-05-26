#ifndef OTA_H
#define OTA_H

#include <esp_err.h>
#include <esp_http_server.h>

esp_err_t perform_ota_update(httpd_req_t *req);

#endif // OTA_H
