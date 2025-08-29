idf_component_register(
    SRCS "interface.c"
    INCLUDE_DIRS 
        "."           # 当前目录（如果有头文件）
        "common"      # 包含 common 子目录
        "minimal"     # 包含 minimal 子目录
        "standard"    # 包含 standard 子目录
        "all"
        "ardupilotmega"
        "ASLUAV"
        "AVSSUAS"
        "csAirLink"
        "cubepilot"
        "development"
        "icarous"
        "loweheiser"
        "python_array_test"
        "storm32"
        "test"
        "ualberta"
        "uAvionix"
    REQUIRES 
    driver 
    main
)