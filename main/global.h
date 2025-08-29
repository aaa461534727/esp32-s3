
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include <stdint.h>

// 定义 GPS 数据结构体
typedef struct {
    double latitude;    // 纬度（单位：度）
    double longitude;   // 经度（单位：度）
    char timestamp[20]; // 时间戳（格式：YYYY-MM-DD HH:MM:SS）
    float speed;        // 速度（单位：米/秒）
    uint8_t satellites; // 可见卫星数
    bool fix_status;    // 定位状态（true=有效定位，false=无效）
} GPS_Data_t;

extern GPS_Data_t gpsData;         // GPS 数据实例
extern SemaphoreHandle_t gps_Mutex;

//定义网页传递进来的数据结构体
typedef struct {
    char basic_id[21];      // 基本ID（20字符+结束符）
    char custom_desc[18];   // 自定义描述（17字符+结束符）
    char run_desc[23];      // 运行描述（22字符+结束符）
    char operator_info[20]; // 操作人（19字符+结束符）
    int client_socket;
} WebCommand_t;
extern WebCommand_t webdata;         // web 数据实例

#endif