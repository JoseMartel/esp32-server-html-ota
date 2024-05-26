#ifndef PAGE_HANDLERS_H
#define PAGE_HANDLERS_H

#include <esp_http_server.h>

esp_err_t index_html_get_handler(httpd_req_t *req);
esp_err_t about_html_get_handler(httpd_req_t *req);
esp_err_t upload_post_handler(httpd_req_t *req);

void register_uri_handlers(httpd_handle_t server);

#endif // PAGE_HANDLERS_H
