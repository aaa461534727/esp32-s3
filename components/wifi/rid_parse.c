#include "rid_parse.h"
// 16进制转字符串
void hex_array_to_string(char *str, unsigned char *hex, int hex_len)
{
    int i = 0;
    for (i = 0; i < hex_len; i++) {
        snprintf(str + i * 2, 3, "%02X", hex[i]);
    }
    str[hex_len * 2] = '\0'; // 添加字符串结束标记
}

// 蓝牙mac地址解析
int rid_parse_ble_mac(unsigned char *data, int offset, struct rid_info *info)
{
    int i = 0;
    unsigned char buf[6];
    // 提取蓝牙mac地址字节形式,小端转大端
    for (i = 0; i < 6; i++)
        // info->ble_mac[i] = data[offset + 5 - i]; //16进制模式,对应的数组大小对应6
        buf[i] = data[offset + 5 - i];
    // 转成字符串，相应的存储数组大小要增加
    hex_array_to_string(info->ble_mac, buf, sizeof(buf) / sizeof(buf[0]));
    //printf("---------------%s --------",info->ble_mac);
    offset += 6;
    return offset;
}

// 运行人ID报文 0x5
int rid_parse_operator_id(unsigned char *data, int offset, struct rid_info *info)
{
    int i = 0;
    int type = 0;
    char buf[20] = {0};
    offset++; // 跳过报文类型字节

    // 运行人ID类型
    type = data[offset];
    info->operator_id_type = type;
    offset++;

    // 运行人ID：ASCII 字符描述
    for (i = 0; i < 20; i++) {
        if (data[offset + i] == 0x0) // null 空字符在长度20字节时，被用于填充
            break;
        snprintf(buf + i, sizeof(buf), "%c", data[offset + i]);
    }
    strcpy(info->operator_id, buf);
    offset += 20;

    // 预留
    offset += 3;

    return offset;
}

// 系统报文 0x4
/* 42 09 00 00 00 00 00 00 00 00 01 00 00 D0 07 D0 07 12 D0 07 CD 7A 05 0B 00*/
int rid_parse_system_info(unsigned char *data, int offset, struct rid_info *info)
{
    int zone = 0, Location_type = 0, areacount = 0, i = 0;
    float latitude = 0, longitude = 0;
    float user_high = 0;
    float radius;
    float area_ceiling = 0, area_floor = 0;
    int category = 0, class = 0;
    float operator_altitude = 0;
    unsigned long timestamp = 0;
    // 打印进到这里的数据
    //  for (i = 0; i < 25; i++) {
    //      printf("%02X ", data[offset + i]);
    //  }
    //  printf("\n");
    offset++; // 跳过报文类型字节

    // 标志位
    // 3 bit,无人机等级分类归属区域取值：0- 未定义；1- 欧洲；2- 中国；3-7：保留
    zone = (data[offset] >> 2) & 7;
    info->zone = zone;

    // 2 bit,无人机操作员的位置类型及取值：0-起飞位置；1-动态位置；2-固定位置
    Location_type = data[offset] & 3;
    info->operator_type = Location_type;

    offset++;

    // 无人机操作员纬度：4 byte
    // printf("[");
    latitude = (data[offset] << 0 | data[offset + 1] << 8 | data[offset + 2] << 16 | data[offset + 3] << 24) * pow(10, -7);
    info->ilotLat = latitude;
    // printf("纬度：%.7f ", latitude);
    offset += 4;

    // 无人机操作经度：4 byte
    longitude = (data[offset] << 0 | data[offset + 1] << 8 | data[offset + 2] << 16 | data[offset + 3] << 24) * pow(10, -7);
    info->ilotLon = longitude;
    // printf("经度：%.7f ", longitude);
    offset += 4;

    // 运行区域计数：2 byte
    areacount = data[offset] << 0 | data[offset + 1] << 8;
    info->areacount = areacount;
    //printf("----%02X--%02X-- areacount %d\n ",data[offset],data[offset+1], areacount);
    offset += 2;

    // 运行区域半径：1 byte
    radius = data[offset] / 10;
    info->distance = radius;
    offset++;
    // printf("距离：%.1f ", info->distance);

    // 运行区域高度上限：2 byte        
    
    area_ceiling = ((data[offset] << 0 | data[offset + 1] << 8) - 2000) * 0.5;  
    //printf("----%02X--%02X-- area_ceiling %f\n ",data[offset],data[offset+1], area_ceiling);
    info->area_ceiling = area_ceiling;
    offset += 2;

    // 运行区域高度下限：2 byte   
    area_floor = ((data[offset] << 0 | data[offset + 1] << 8) - 2000) * 0.5;
    //printf("----%02X--%02X-- area_floor %f\n ",data[offset],data[offset+1], area_floor);
    info->area_floor = area_floor;
    offset += 2;

    // UA运行类别/UA等级：1 byte
    category = data[offset] >> 4;
    class = data[offset] & 0x0F;
    // 无人机类别//无人机等级
    if (category == 0) {
        info->category = category;
        info->class = 0;

    } else if (category == 1) {

        info->category = category;
        info->class = class;

    } else if (category == 2) {
        info->category = category;
        info->class = 0;

    } else if (category == 3) {
        info->category = category;
        info->class = 0;

    }
    offset++;

    // 无人机操作员高度：2 byte
    user_high = (data[offset] << 0 | data[offset + 1] << 8) * 0.5 - 1000;
    // printf("无人机操作员高度: %.1f ", user_high);
    info->operator_altitude = user_high;
    offset += 2;

    // 时间戳：4 byte
    timestamp = (data[offset] << 0) | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24);
    struct tm *utc_time = gmtime(&timestamp); // 转换为UTC时间
    sprintf(info->sys_timestamp, "%d-%02d-%02d %02d:%02d:%02d", utc_time->tm_year + 1900,
            utc_time->tm_mon + 1,
            utc_time->tm_mday,
            utc_time->tm_hour,
            utc_time->tm_min,
            utc_time->tm_sec);
    // printf("系统时间：%s\n", info->sys_timestamp);
    offset += 4;

    // 预留：1 byte
    offset++;

    // printf("]\n");

    return offset;
}

// 运行描述报文 0x3
int rid_parse_run_comment(unsigned char *data, int offset, struct rid_info *info)
{
    int comment_type = 0, i = 0;
    char buf[24] = {0};

    offset++; // 跳过报文类型字节

    // selfid描述类型
    comment_type = data[offset];
    info->selfid_type = comment_type;

    offset++;

    // 提取描述信息
    for (i = 0; i < 23; i++) { // 23 byte ASCII 字符
        snprintf(buf + i, sizeof(buf) - 1, "%c", data[offset]);
        offset++;
    }
    strcpy(info->selfid, buf);
    // printf("comment=%s\n", buf);

    return offset;
}

// 位置向量报文 0x1
/* 12 10 00 00 00 00 00 00 00 00 00 00 00 D0 07 D0 07 D0 07 29 02 22 42 01 00 */
int rid_parse_location_vector(unsigned char *data, int offset, struct rid_info *info)
{
    int i = 0;
    char status = 0, high_type = 0, ew_flag = 0, speed_param = 0;
    float g_speed = 0, v_speed = 0, latitude = 0, longitude = 0;
    float air_high = 0, geo_high = 0, g_high = 0;
    char vertical = 0, horizontal = 0;            // 水平垂直精度
    char b_alt_acc = 0, speed_acc = 0;            // 气压高度，速度精度
    unsigned long timestamp = 0, ValueTenths = 0; // 时间戳
    unsigned int timestamp_vale = 0;
    time_t now;
    struct tm *local;

    offset++; // 跳过报文类型字节

    status = data[offset] >> 4; // 运行状态
    info->status = status;

    // 高度类型位：1-基于AGL高度;0-基于起飞地的高度
    high_type = (data[offset] & (1 << 2)) ? 1 : 0;
    info->high_type = high_type;

    // 航迹角E/W方向标志：0 <180; 1 >=180
    ew_flag = (data[offset] & (1 << 1)) ? 1 : 0;
    info->ew_flag = ew_flag;
    // if (ew_flag == 0)
    //     strcpy(info->ew_flag, "<180");
    // else if (ew_flag == 1)
    //     strcpy(info->ew_flag, ">=180");

    // 用于计算无人机速度
    speed_param = (data[offset] & (1 << 0)) ? 1 : 0; // 速度乘数: 0: x 0.25;1: x 0.75
    offset++;

    // 航迹角(方向):1 byte
    int ew = data[offset];
    if (ew_flag == 1)
        ew += 180;
    info->direction = ew;
    offset++;
    // printf("[航迹角：%d ", info->direction);//方向

    // 地速（速度）:1 byte
    if (speed_param == 0)
        g_speed = data[offset] * 0.25;
    else
        g_speed = data[offset] * 0.75 + 63.75;
    if (g_speed == 255)
        printf("value is unknown "); // 英文手册上写，真实数据是未知的
    else if (g_speed == 254.25)
        printf("speed is at least 254.25"); // 速度至少为254.25
    info->speed = g_speed;
    offset++;
    // printf("地速: %.2f ", g_speed);

    // 垂直速度:1 byte
    v_speed = data[offset] * 0.5;
    info->v_speed = v_speed;
    offset++;
    // printf("垂直速度: %.1f ", v_speed);

    // 无人机纬度：4 byte
    latitude = (data[offset] << 0 | data[offset + 1] << 8 | data[offset + 2] << 16 | data[offset + 3] << 24) * pow(10, -7);
    info->lat = latitude;
    // printf("纬度：%.7f ", latitude);
    offset += 4;

    // 无人机经度：4 byte
    longitude = (data[offset] << 0 | data[offset + 1] << 8 | data[offset + 2] << 16 | data[offset + 3] << 24) * pow(10, -7);
    info->lon = longitude;
    // printf("经度：%.7f ", longitude);
    offset += 4;

    // 气压高度：2 byte
    air_high = (data[offset] << 0 | data[offset + 1] << 8) * 0.5 - 1000;
    //  printf("气压高度：%.1f ", air_high);
    info->air_high = air_high;
    offset += 2;

    // 大地高度：2 byte
    geo_high = (data[offset] << 0 | data[offset + 1] << 8) * 0.5 - 1000;
    info->alt = geo_high;
    //  printf("大地高度：%.1f ", geo_high);
    offset += 2;

    // 无人机距地高度：2 byte  		(基于起飞地的高度)
    g_high = (data[offset] << 0 | data[offset + 1] << 8) * 0.5 - 1000;
    if (g_high == -1000) {
        printf("real value is unknown "); //英文手册上写，真实数据是未知的
    } else {
        // printf("距地高度：%.1f ", g_high);
        info->g_high = g_high;
    }
    offset += 2;

    // 水平垂直精度：1 byte
    vertical = data[offset] >> 4;    // 垂直
    horizontal = data[offset] & 0xF; // 水平
    info->hor_accuracy = horizontal;
    info->ver_accuracy = vertical;
    offset++;

    // 气压高度精度
    b_alt_acc = data[offset] >> 4;
    info->alt_accuracy = b_alt_acc;

    // 速度精度
    speed_acc = data[offset] & 0xF;
    info->speed_accuracy = speed_acc;

    offset++;

    // 时间戳：2 byte
    timestamp = (data[offset] << 0 | data[offset + 1] << 8);
    info->timestamp = timestamp;
#if 0
    now = time(NULL);
    struct tm utc_base;
    gmtime_r(&now, &utc_base); //使用UTC时间

    // 计算当前小时已过的十分之一秒数
    unsigned int threshold = (utc_base.tm_min * 60 + utc_base.tm_sec) * 10;
    unsigned int encoded_value = timestamp;

    struct tm decoded_tm;
    if (encoded_value > threshold) {// 回滚到上一小时（处理跨日）
        time_t prev_hour = mktime(&utc_base) - 3600;
        gmtime_r(&prev_hour, &decoded_tm);
    } else {
        decoded_tm = utc_base;
    }

    // 基准时间设为整点重置分钟/秒
    decoded_tm.tm_min = 0;
    decoded_tm.tm_sec = 0;

    // 计算总时间（含0.1秒精度）
    double total_seconds = encoded_value / 10.0;
    time_t base_epoch = mktime(&decoded_tm);
    time_t final_epoch = base_epoch + (time_t)total_seconds;
    struct tm final_tm;
    gmtime_r(&final_epoch, &final_tm);

    printf("timestamp:%d ==> UTC: %02d:%02d:%04.1f\n",
           timestamp,
           final_tm.tm_hour,
           final_tm.tm_min,
           final_tm.tm_sec + (encoded_value % 10) * 0.1);
#endif
    offset += 2;

    // 时间戳精度：1 byte
    info->time_acc = data[offset]& 0x0F;  // 取低四位
    //printf("time_acc: %02X\n", info->time_acc);
    offset++;

    // 预留：1byte
    offset++;

    // printf("]");

    return offset;
}

// 基本ID报文 0x0
/* [02][11][32][30][35][31][46][45][41][42][50][54][30][30][30][30][30][30][30][31][33][35][00] */
int rid_parse_basic_id(unsigned char *data, int offset, struct rid_info *info)
{
    int i = 0;
    int id_type = 0, ua_type = 0;
    char buf[24] = {0}; // 保存ID
    offset++;           // 跳过报文类型字节

    id_type = data[offset] >> 4;
    info->id_type = id_type;

    // 设置id类型字符串
    if (id_type == 0)
        strcpy(info->id_type_str, "None");
    else if (id_type == 1)
        strcpy(info->id_type_str, "Serial Number");
    else if (id_type == 2)
        strcpy(info->id_type_str, "CAA Assigned Registration ID");
    else if (id_type == 3)
        strcpy(info->id_type_str, "UTM Assigned UUID");
    else if (id_type == 4)
        strcpy(info->id_type_str, "Specific Session ID");
    // 设置ua类型字符串
    ua_type = data[offset] & 0xF;
    info->ua_type = ua_type;

    if (ua_type == 0)
        strcpy(info->ua_type_str, "None");
    else if (ua_type == 1)
        strcpy(info->ua_type_str, "Aeroplane");
    else if (ua_type == 2)
        strcpy(info->ua_type_str, "Helicopter or Multirotor");
    else if (ua_type == 3)
        strcpy(info->ua_type_str, "Gyroplane");
    else if (ua_type == 4)
        strcpy(info->ua_type_str, "Hybrid Lift");
    else if (ua_type == 5)
        strcpy(info->ua_type_str, "Ornithopter");
    else if (ua_type == 6)
        strcpy(info->ua_type_str, "Glider");
    else if (ua_type == 7)
        strcpy(info->ua_type_str, "Kite");
    else if (ua_type == 8)
        strcpy(info->ua_type_str, "Free Balloon");
    else if (ua_type == 9)
        strcpy(info->ua_type_str, "Captive Balloon");
    else if (ua_type == 10)
        strcpy(info->ua_type_str, "Airship");
    else if (ua_type == 11)
        strcpy(info->ua_type_str, "Free Fall or Parachute");
    else if (ua_type == 12)
        strcpy(info->ua_type_str, "Rocket");
    else if (ua_type == 13)
        strcpy(info->ua_type_str, "Tethered Powered Aircraft");
    else if (ua_type == 14)
        strcpy(info->ua_type_str, "Ground Obstacle");
    else if (ua_type == 15)
        strcpy(info->ua_type_str, "Other");
    // printf("----------------id_type %d--------ua_type %d-----\n",id_type,ua_type);
    offset++;

    // 设置唯一ID
    for (i = 0; i < 20; i++) {
        if (data[offset + i] == 0x0) // null 空字符在长度20字节时，被用于填充
            break;
        snprintf(buf + i, sizeof(buf), "%c", data[offset + i]);
    }
    if (id_type == 1) {
        strcpy(info->sn, buf);
    } else if (id_type == 2) {
        strcpy(info->uas_id, buf);
    } else if (id_type == 3) {
        strcpy(info->uuid, buf);
    } else if (id_type == 4) {
        strcpy(info->seid, buf);
    }

    // printf("[UAS ID:%s]", info->sn);
    offset += 20;

    // 3个预留字节
    offset += 3;

    return offset;
}
// 自定义报文0x2
int rid_parse_reserve(unsigned char *data, int offset, struct rid_info *info) 
{
    int authtype, page_index, i;
    int total_length,page_number; // 自定义认证信息长度
    unsigned long timestamp;
    char buf[24] = {0};  // 增大缓冲区至24字节（23数据+1终止符）
    offset++;           // 跳过报文类型字节

    // 解析公共头部
    authtype = data[offset] >> 4;       // 认证类型（高4位）
    page_number = data[offset] & 0xF;   // 当前消息的索引（低4位）
    offset++;

    if(page_number != 0)
    {
        // 安全复制23字节数据
        int len = 0;
        for (i = 0; i < 23; i++) {
            if (data[offset + i] == 0x0)
                break;
            // 直接赋值
            buf[i] = data[offset + i];
            len = i + 1;
        }
        buf[len] = '\0';  // 确保终止
        
        // 检查目标缓冲区剩余空间
        int current_len = strlen(info->authentication_data);
        int remain_space = sizeof(info->authentication_data) - current_len - 1;
        
        if (remain_space > 0) {
            // 安全追加数据
            strncat(info->authentication_data, buf, remain_space);
        }

        offset += 23;
        return offset;
    }

    page_index = data[offset] & 0xF;    // 页索引（低4位）
    offset++;
    
    total_length = data[offset];        // 认证数据总长度
    offset++;

    // 存储公共信息
    info->authtype = authtype;
    info->auth_total_length = total_length;

    // 解析时间戳 (4字节)
    timestamp = (unsigned long)data[offset] << 24 | 
                (unsigned long)data[offset+1] << 16 | 
                (unsigned long)data[offset+2] << 8 | 
                (unsigned long)data[offset+3];
    // 转换时间戳为可读格式
    time_t ts = (time_t)timestamp;
    struct tm *utc_time = gmtime(&ts);
    sprintf(info->aut_timestamp, "%d-%02d-%02d %02d:%02d:%02d",
            utc_time->tm_year + 1900,
            utc_time->tm_mon + 1,
            utc_time->tm_mday,
            utc_time->tm_hour,
            utc_time->tm_min,
            utc_time->tm_sec);            
    offset += 4;

    //ASCII 字符描述
    for (i = 0; i < 17; i++) {
        if (data[offset + i] == 0x0)
            break;
        snprintf(buf + i, sizeof(buf), "%c", data[offset + i]);
    }
    strcpy(info->authentication_data, buf);
    offset += 17;

    return offset;
}
/*----------------------get distance section---------------------------------*/
#define PI                      3.1415926
#define EARTH_RADIUS            6378.137        //地球近似半径

float radian(double d);
float _get_distance(double lat1, double lng1, double lat2, double lng2);

// 求弧度
float radian(double d)
{
    return d * PI / 180.0;   //角度1˚ = π / 180
}

//计算距离
float _get_distance(double lat1, double lng1, double lat2, double lng2)
{
    float radLat1 = radian(lat1);
    float radLat2 = radian(lat2);
    float a = radLat1 - radLat2;
    float b = radian(lng1) - radian(lng2);
    
    float dst = 2 * asin((sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2) )));
    
    dst = dst * EARTH_RADIUS;
    dst= round(dst * 10000) / 10000; 
    return dst;
}

void get_distance(struct rid_info *data, char *buf)
{	
	float lat1 = data->lat;
	float lng1 = data->lon;

	float lat2 = data->ilotLat;
	float lng2 = data->ilotLon;

	float dst = _get_distance(lat1, lng1, lat2, lng2);

	sprintf(buf, "%0.1f", (dst * 1000));//km->m
	
	return ; 
}
void ble_get_distance(struct rid_info data, char *buf)
{	
	float lat1 = data.lat;
	float lng1 = data.lon;

	float lat2 = data.ilotLat;
	float lng2 = data.ilotLon;

	float dst = _get_distance(lat1, lng1, lat2, lng2);

	sprintf(buf, "%0.1f", (dst * 1000));//km->m
	
	return ; 
}
/*----------------get distance end----------------------------*/
