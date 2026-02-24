#ifndef RID_WIFI_SNIFFER_H
#define RID_WIFI_SNIFFER_H

#include "lwip/ip4_addr.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "lwip/lwip_napt.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "global.h"
#include "../libopendroneid/opendroneid.h"
#include "../udp/udp_client.h"
#include "../4G/ec200x.h"
#include "rid_parse.h"
#include "cJSON.h"

int rid_wifi_sniffer_get_last_data(char *out_buf, size_t buf_size);
void rid_wifi_sniffer_init(void);
void free_rid_wifi_sniffer(void);
#define RID_SN_LIST_FILE    "/spiffs/rid_sn_list"
#define PACKAGE_LENGTH		0x19 //每条报文长度固定值
#define MAX_PAYLOAD 2048

typedef struct Node {
    // 数据域
    rid_info_t data;
    // 指针域
    struct Node *next;
} Node;

struct device_Info{
	bool status;
	char id[24];
    int rid_port; //udp port
    char rid_server[64]; //服务器地址
    float dev_lat;  //接受站经度
    float dev_lon; //接收站纬度
    char model[24]; 
    char device_id[32];//lan口地址
};

#endif
