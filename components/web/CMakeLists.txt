# components/web/CMakeLists.txt
idf_component_register(
    SRCS "web_server.c" 
    INCLUDE_DIRS "."
    EMBED_FILES "html/index.html"
    REQUIRES 
        esp_http_server
        lwip
        driver
        main
        nvs_flash
)