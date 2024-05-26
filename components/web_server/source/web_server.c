#include "web_server.h"
#include "page_handlers.h"
#include <esp_log.h>

// static const char *TAG = "WebServer";

void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = MAX_HTTP_URI_LEN;
    config.max_resp_headers = MAX_HTTP_HDR_LEN;

    if (httpd_start(&server, &config) == ESP_OK) {
        register_uri_handlers(server);
    }
}
