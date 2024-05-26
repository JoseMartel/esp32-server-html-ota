idf_component_register(SRCS "source/ota.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_server app_update)

