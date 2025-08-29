# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# 合并所有组件路径（注意路径分隔符）
set(EXTRA_COMPONENT_DIRS 
    "./components/libopendroneid"
    "./components/ble"
    "./components/wifi"
    "./components/uart"
    "./components/web"
    "./components/mavlink"
    "./components/power"
)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-s3-remoteid)
