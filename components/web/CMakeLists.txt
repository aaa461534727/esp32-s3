# components/web/CMakeLists.txt
idf_component_register(
    SRCS "web_server.c" 
    INCLUDE_DIRS "."
    EMBED_FILES
        "html/home.html"
        "html/manage.html"
        "html/ota.html"
        "html/display.html"
    REQUIRES 
        esp_http_server
        lwip
        driver
        main
        nvs_flash
        iot
        udp
        wifi
)