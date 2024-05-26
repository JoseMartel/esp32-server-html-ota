idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ota web_server utilities nvs_flash esp_netif esp_event esp_http_server)
