idf_component_register(
    SRCS "iot.c" "iot_interaction.c"
    INCLUDE_DIRS  "."
    REQUIRES 
        driver
        4G
        main
        esp_netif
        lwip  
        esp_event
)
