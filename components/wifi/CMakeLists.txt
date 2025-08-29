idf_component_register(
    SRCS "udp_clicent.c" "wifi_ap.c" "wifi.c" 
    INCLUDE_DIRS  "."
    REQUIRES    # 核心依赖
        libopendroneid
        main
        esp_wifi       # 核心依赖
        driver         # 底层驱动（esp_wifi可能依赖）
        esp_netif      # 网络接口（部分WiFi功能需要
)
