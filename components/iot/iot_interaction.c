#include "iot_interaction.h"

static const char *TAG = "iot-interaction";

#ifndef IOTM_BIND_CODE
#define IOTM_BIND_CODE     "d84cee90bbc82625"
#endif

QueueHandle_t sysinfo_queue = NULL;
TaskHandle_t sysinfo_task_handle = NULL;

//publish/subscribe 规则
char subscribeTmp[128] = {0};
char publishTmp[128]   = {0}; // 默认 device/report

esp_mqtt_client_handle_t g_client = NULL;

// -------------------- 主题生成：device/<mac_no_colon_lower> --------------------
void build_topics_from_mac(void)
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char macbuf[13] = {0};
    snprintf(macbuf, sizeof(macbuf),
             "%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    snprintf(subscribeTmp, sizeof(subscribeTmp), "device/%s", macbuf);
    snprintf(publishTmp,   sizeof(publishTmp),   "%s", "device/report");

    ESP_LOGI(TAG, "subscribeTmp = %s", subscribeTmp);
    ESP_LOGI(TAG, "publishTmp   = %s", publishTmp);
}

// -------------------- 获取 Wi-Fi IP/mask/gw --------------------
static void get_wifi_ipv4(char *ip, size_t iplen,
                          char *mask, size_t masklen,
                          char *gw, size_t gwlen)
{
    if(ip && iplen)   ip[0] = 0;
    if(mask && masklen) mask[0] = 0;
    if(gw && gwlen)   gw[0] = 0;

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if(!netif) return;

    esp_netif_ip_info_t info;
    if(esp_netif_get_ip_info(netif, &info) != ESP_OK) return;

    snprintf(ip,   iplen,   IPSTR, IP2STR(&info.ip));
    snprintf(mask, masklen, IPSTR, IP2STR(&info.netmask));
    snprintf(gw,   gwlen,   IPSTR, IP2STR(&info.gw));
}

// -------------------- MQTT publish 帮助函数 --------------------
static int mqtt_publish_text(esp_mqtt_client_handle_t client,
                             const char *topic,
                             const char *payload,
                             int qos, int retain)
{
    int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, qos, retain);
    //ESP_LOGI(TAG, "publish topic=%s msg_id=%d payload=%s", topic, msg_id, payload);
    return msg_id;
}

// -------------------- 你的 local_bind_iotm：ESP32 版本（核心：action=bind） --------------------
void local_bind_iotm(esp_mqtt_client_handle_t client)
{
    // 1) 基础字段：action/mac
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char mac_colon[18] = {0};
    snprintf(mac_colon, sizeof(mac_colon),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    cJSON *obj_all = cJSON_CreateObject();
    cJSON_AddStringToObject(obj_all, "action", "bind");
    cJSON_AddStringToObject(obj_all, "mac", mac_colon);

    // 2) data 对象
    cJSON *obj_data = cJSON_CreateObject();
    cJSON_AddItemToObject(obj_all, "data", obj_data);

    // bindcode / gatewayIsReset / csid / model / fwver
    cJSON_AddStringToObject(obj_data, "bindcode", IOTM_BIND_CODE);
    cJSON_AddStringToObject(obj_data, "gatewayIsReset", "1");

    // 你后续可以改成 menuconfig 或 NVS 或编译宏
    cJSON_AddStringToObject(obj_data, "csid",  "");                 // 原 UCI custom/csid
    cJSON_AddStringToObject(obj_data, "model", "esp32-s3");         // 原 sysinfo/soft_model

    // fwver：用 IDF app desc
    const esp_app_desc_t *app_desc = esp_app_get_description();
    cJSON_AddStringToObject(obj_data, "fwver", app_desc ? app_desc->version : "");

    // 3) IP/mask/gw（ESP32 用 PPP）
    char ip[16], mask[16], gw[16];
    get_ppp_ipv4_gw_mask(ip, sizeof(ip), mask, sizeof(mask), gw, sizeof(gw));

    cJSON_AddStringToObject(obj_data, "ip",   ip);
    cJSON_AddStringToObject(obj_data, "mask", mask);
    cJSON_AddStringToObject(obj_data, "gw",   gw);

    // network：你 Linux 里 pub/priv（SIM），ESP32 先固定 pub
    cJSON_AddStringToObject(obj_data, "network", "pub");

    // 4) gps 对象（ESP32 没 GPS 模块就按你平台格式置 0）
    cJSON *obj_gps = cJSON_CreateObject();
    cJSON_AddItemToObject(obj_data, "gps", obj_gps);
    cJSON_AddNumberToObject(obj_gps, "gps_support", 0);
    // 如果你平台一定要字段，也可以加：
    // cJSON_AddStringToObject(obj_gps, "longitude", "0");
    // cJSON_AddStringToObject(obj_gps, "latitude",  "0");

    // 5) lte_info 对象（ESP32 没 LTE 模块就按你平台格式置 0）
    cJSON *obj_lte = cJSON_CreateObject();
    cJSON_AddItemToObject(obj_data, "lte_info", obj_lte);

    cJSON_AddStringToObject(obj_lte, "ip", ip[0] ? ip : ""); // 没 LTE 时用 Wi-Fi IP
    cJSON_AddStringToObject(obj_lte, "iccid", "0");
    cJSON_AddStringToObject(obj_lte, "eci",   "0");
    cJSON_AddStringToObject(obj_lte, "sinr",  "0");
    cJSON_AddStringToObject(obj_lte, "flow_up",   "0");
    cJSON_AddStringToObject(obj_lte, "flow_down", "0");
    cJSON_AddStringToObject(obj_lte, "isp", "0");

    // 6) 序列化并发布到 device/report
    char *payload = cJSON_PrintUnformatted(obj_all);
    if(payload){
        mqtt_publish_text(client, publishTmp, payload, 2, 0);
        cJSON_free(payload);
    }else{
        ESP_LOGE(TAG, "cJSON_PrintUnformatted failed");
    }

    cJSON_Delete(obj_all);
}

// 获取系统运行时间（毫秒）
void getSysUptime(char *buf, size_t len)
{
    // 获取自启动以来的微秒数（int64_t）
    int64_t uptime_us = esp_timer_get_time();
    // 转换为秒
    uint64_t uptime_sec = uptime_us / 1000000ULL;

    unsigned long days, hours, minutes, seconds;

    seconds = (unsigned long)(uptime_sec % 60);
    uptime_sec /= 60;
    minutes = (unsigned long)(uptime_sec % 60);
    uptime_sec /= 60;
    hours   = (unsigned long)(uptime_sec % 24);
    uptime_sec /= 24;
    days    = (unsigned long)uptime_sec;

    // 格式化为 "天;时;分;秒"
    snprintf(buf, len, "%lu;%lu;%lu;%lu", days, hours, minutes, seconds);
}

// 获取当前设备的 MAC 地址
static void getDeviceMac(char *mac_str, size_t len)
{
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    snprintf(mac_str, len, "%02x:%02x:%02x:%02x:%02x:%02x", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// 获取系统信息并发布
static void getSysInfo(void)
{
    char time[64] = {0};
    char mac[64] = {0};

    // 获取系统运行时间和 MAC 地址
    getSysUptime(time, sizeof(time));
    getDeviceMac(mac, sizeof(mac));

    // 创建 JSON 对象
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "action", "getSysInfo");
    cJSON_AddStringToObject(obj, "mac", mac);
    cJSON_AddStringToObject(obj, "upTime", time);

    // 序列化 JSON 对象为字符串
    char *json_str = cJSON_PrintUnformatted(obj);

    if (json_str) {
        // 发布数据到 MQTT
        mqtt_publish_text(g_client, publishTmp, json_str, 2, 0);
        // ESP_LOGI(TAG, "Published: %s", json_str);

        // 释放 JSON 字符串
        cJSON_free(json_str);
    } else {
        ESP_LOGE(TAG, "Failed to create JSON string");
    }

    // 释放 JSON 对象
    cJSON_Delete(obj);
}

void timer_callback(TimerHandle_t xTimer) {
    // 关键：若队列已被删除或 MQTT 客户端已销毁，直接返回
    if (sysinfo_queue == NULL || g_client == NULL) {
        return;
    }
    static uint32_t count = 0;
    count++;

    if (count % 5 == 0) {
        // 发送一个空信号唤醒工作任务
        uint32_t val = 1;
        xQueueSend(sysinfo_queue, &val, 0);   // 0 表示不等待
    }

    if (count % 10 == 0) {
        ESP_LOGI(TAG, "Performing a task every 10 seconds.");
    }
}
void sysinfo_task(void *pvParameters) {
    while (1) {
        uint32_t dummy;  // 仅作为唤醒信号
        if (xQueueReceive(sysinfo_queue, &dummy, portMAX_DELAY)) {
            getSysInfo();   // 耗时操作在专用任务中执行，栈大小可单独配置
        }
    }
}

// -------------------- 收消息（如果你暂时只要能连接+bind，这块也可不动） --------------------
void handle_incoming_json(esp_mqtt_client_handle_t client, const char *data, int len)
{
    char *buf = (char*)calloc(1, len + 1);
    if(!buf) return;
    memcpy(buf, data, len);

    cJSON *root = cJSON_Parse(buf);
    if(!root){
        ESP_LOGE(TAG, "JSON parse failed: %s", buf);
        free(buf);
        return;
    }
    // 从 JSON 中获取 "topicurl" 字段
    const cJSON *topicurl = cJSON_GetObjectItem(root, "topicurl");
    if (cJSON_IsString(topicurl)) {
        const char *cmd = topicurl->valuestring;
        ESP_LOGI(TAG, "Received command: %s", cmd);
        // ----- 根据不同的 topicurl 执行不同的业务逻辑 -----
        if (strcmp(cmd, "getRidConfig") == 0) {
            // 处理 getRidConfig 命令
            ESP_LOGI(TAG, "Handling getRidConfig...");
        }
        else if (strcmp(cmd, "getIotMCfg") == 0) {
            // 处理 getIotMCfg 命令
            ESP_LOGI(TAG, "Handling getIotMCfg...");
        }
        else {
            // ESP_LOGW(TAG, "Unknown topicurl: %s", cmd);
        }
    } else {
        ESP_LOGW(TAG, "No 'topicurl' field in JSON");
    }

    cJSON_Delete(root);
    free(buf);
    (void)client;
}
