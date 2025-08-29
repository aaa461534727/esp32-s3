idf_component_register(
    SRCS "ble_peripheral.c"        # 确保路径正确（若在子目录需加相对路径）
    INCLUDE_DIRS "."               # 包含当前目录的头文件
    REQUIRES 
    main
    libopendroneid
    bt                 # 蓝牙核心组件（关键修正）
    driver             # 基础驱动
)