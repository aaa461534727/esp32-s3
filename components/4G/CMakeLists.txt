idf_component_register(
    SRCS "ec200x.c" 
    INCLUDE_DIRS  "."
    REQUIRES
        driver 
        main
        udp
        esp_netif
        lwip  
        esp_event
)  
