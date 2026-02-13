#ifndef RID_PARSER_H
#define RID_PARSER_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define	BASID_ID			0x0
#define LOCATION_VECTOR		0x1
#define	RESERV				0x2
#define RUN_COMMENT			0x3
#define SYSTEM				0x4
#define OPERATOR_ID			0x5

#define DATA_BUF_LEN		(1024 *5)


// #pragma pack(4)
typedef struct rid_info {

    int status;             // 0x1 飞行器状态
    int direction;          // 0x1 方向
    float lon;              // 0x1 经度
    float lat;              // 0x1 纬度
    float alt;              // 0x1 高度
    float speed;            // 0x1 水平速度
    float v_speed;          // 0x1 垂直速度
    float air_high;         // 0x1 气压高度
    float geo_high;         // 0x1 大地高度
    char high_type;         // 0x1 高度类型
    char ew_flag;           // 0x1 航向角标志类型
    float g_high;           // 0x1 无人机高度
    char hor_accuracy;      // 0x1 水平精度
    char ver_accuracy;      // 0x1 垂直精度
    char alt_accuracy;      // 0x1 气压高度精度
    char speed_accuracy;    // 0x1 速度精度
    unsigned int timestamp; // 0x1 时间戳
    char time_acc;         // 0x1 时间戳精度

    char authtype;                  // 0x2认证类型
    int auth_total_length;          // 0x2认证数据总长度
    char aut_timestamp[128];         // 0x2时间戳(仅页0有效)
    char authentication_data[257];  // 0x2认证数据缓冲区(17+15*16=257)

    char selfid_type; // 0x3 selfid类型
    char selfid[24];  // 0x3 selfid内容

    char zone;               // 0x4 无人机区域
    char operator_type;      // 0x4 操作员位置类型
    float distance;          // 0x4 操作人距离
    float ilotLon;           // 0x4  操作人经度
    float ilotLat;           // 0x4 操作人纬度
    int areacount;           // 0x4 运行区域计数
    float area_ceiling;      // 0x4运行区域高度上限
    float area_floor;        // 0x4 运行区域高度下限
    char category;           // 0x4 无人机类别
    char class;              // 0x4 无人机等级
    float operator_altitude; // 0x4 操作员高度
    char sys_timestamp[128];  // 0x4系统信息时间戳

    int operator_id_type; // 0x5 操作员ID类型
    char operator_id[24]; // 0x5 操作员ID

    int id_type;          // 0x0 id类型
    int ua_type;          // 0x0 ua类型
    char id_type_str[40]; // 0x0 id类型字符串
    char ua_type_str[40]; // 0x0 ua类型字符串
    char uas_id[24];      // 0x0 4种唯一id
    char sn[24];          // 0x0
    char uuid[24];        // 0x0
    char seid[24];        // 0x0
    char model[64];       // mtk7621 mac
    // 蓝牙专用
    char ble_mac[24]; // 蓝牙mac
    int msg_type;     // 消息类型

	// wifi
	int8_t rssi; //wifi rssi
    int channel;
	int channel_busy_time; //channel_busy_time
    float loss_rate; //丢包率

} rid_info_t; // 声明结构体

// #pragma pack()

// 蓝牙mac地址解析
extern int rid_parse_ble_mac(unsigned char *data, int offset, struct rid_info *info);
// 基本ID报文
extern int rid_parse_basic_id(unsigned char *data, int offset, struct rid_info *info);
// 自定义报文0x2
extern int rid_parse_reserve(unsigned char *data, int offset, struct rid_info *info);
// 位置向量报文
extern int rid_parse_location_vector(unsigned char *data, int offset, struct rid_info *info);
// 运行描述报文
extern int rid_parse_run_comment(unsigned char *data, int offset, struct rid_info *info);
// 系统报文
extern int rid_parse_system_info(unsigned char *data, int offset, struct rid_info *info);
// 运行人ID报文
extern int rid_parse_operator_id(unsigned char *data, int offset, struct rid_info *info);
//计算经纬度距离
extern void ble_get_distance(struct rid_info data, char *buf);
#endif
