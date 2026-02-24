#ifndef _IOT_INTERACTION_H_
#define _IOT_INTERACTION_H_
#include "iot.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_netif_types.h"
#include "../4G/ec200x.h"
#include "../wifi/rid_wifi_sniffer.h"
// 声明 g_client 为全局变量
extern esp_mqtt_client_handle_t g_client;
extern char subscribeTmp[128];
extern char publishTmp[128];
extern QueueHandle_t sysinfo_queue;
extern TaskHandle_t sysinfo_task_handle;
extern QueueHandle_t ridinfo_queue;
extern TaskHandle_t ridinfo_task_handle;

void sysinfo_task(void *pvParameters);
void ridinfo_task(void *pvParameters);

void build_topics_from_mac(void);
void local_bind_iotm(esp_mqtt_client_handle_t client);
void timer_callback(TimerHandle_t xTimer);
void handle_incoming_json(esp_mqtt_client_handle_t client, const char *data, int len);
#endif
