idf_component_register(
    SRCS "udp_client.c"
    INCLUDE_DIRS  "."
    REQUIRES 
        driver
        4G 
        main
        esp_netif
        lwip  
        esp_event
)
