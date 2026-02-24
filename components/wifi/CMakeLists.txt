idf_component_register(
    SRCS  "wifi_ap.c" "wifi.c" "rid_wifi_sniffer.c" "rid_parse.c"
    INCLUDE_DIRS  "."
    REQUIRES    # 核心依赖
        libopendroneid
        4G
        udp
        main
        esp_wifi       # 核心依赖
        driver         # 底层驱动（esp_wifi可能依赖）
        esp_netif      # 网络接口（部分WiFi功能需要
)

# 创建 SPIFFS 分区镜像，将 fs_data 目录下的文件打包到 storage 分区
spiffs_create_partition_image(storage fs_data FLASH_IN_PROJECT)
