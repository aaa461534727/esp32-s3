#include "iot.h"

// 在文件开头添加全局任务句柄（用于删除 mqtt_manager_task）
static TaskHandle_t mqtt_manager_task_handle = NULL;

static const char *TAG = "iot-mqtt";

#ifndef IOTM_SERVER_HOST
#define IOTM_SERVER_HOST   "iot.cpe-iot.com"
#endif

#ifndef IOTM_SERVER_PORT
#define IOTM_SERVER_PORT   1883
#endif

#ifndef IOTM_KEEP_ALIVE
#define IOTM_KEEP_ALIVE    60
#endif

#ifndef IOTM_USERNAME
#define IOTM_USERNAME      ""     // 需要就填
#endif

#ifndef IOTM_PASSWORD
#define IOTM_PASSWORD      ""     // 需要就填
#endif

static TimerHandle_t xTimer = NULL;

// -------------------- LWT（offline） --------------------
static void mqtt_set_last_will(esp_mqtt_client_config_t *cfg)
{
    // {"action":"offline","mac":"xx:xx:xx:xx:xx:xx"}
    uint8_t mac[6]={0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char mac_colon[18]={0};
    snprintf(mac_colon, sizeof(mac_colon),
            "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "action", "offline");
    cJSON_AddStringToObject(obj, "mac", mac_colon);

    char *tmp = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);

    static char will_buf[160] = {0};
    if(tmp){
        strncpy(will_buf, tmp, sizeof(will_buf)-1);
        cJSON_free(tmp);
    }else{
        strncpy(will_buf, "{\"action\":\"offline\"}", sizeof(will_buf)-1);
    }

    cfg->session.last_will.topic  = publishTmp;
    cfg->session.last_will.msg    = will_buf;
    cfg->session.last_will.qos    = 2;
    cfg->session.last_will.retain = true;
}


// -------------------- MQTT 事件回调 --------------------
static void mqtt_event_handler(void *handler_args,
                            esp_event_base_t base,
                            int32_t event_id,
                            void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch((esp_mqtt_event_id_t)event_id){
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");

            // 订阅 device/<mac>
            esp_mqtt_client_subscribe(client, subscribeTmp, 1);
            ESP_LOGI(TAG, "Subscribed: %s", subscribeTmp);

            // 连接成功立即绑定上报
            local_bind_iotm(client);
            // 创建定时器，每1秒执行一次
            xTimer = xTimerCreate("TaskTimer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, timer_callback);
            if (xTimer != NULL) {
                xTimerStart(xTimer, 0);  // 启动定时器
                ESP_LOGI(TAG, "Timer started");
            } else {
                ESP_LOGE(TAG, "Failed to create timer");
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            // ESP32 不要 exit，让它自动重连
            break;

        case MQTT_EVENT_DATA:
            // ESP_LOGI(TAG, "RX topic=%.*s payload=%.*s",
            //          event->topic_len, event->topic,
            //          event->data_len, event->data);

            handle_incoming_json(client, event->data, event->data_len);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            break;
    }
}

// -------------------- 启动 MQTT --------------------
static void mqtt_start(void)
{
    build_topics_from_mac();

    // 拼 URI：mqtt://iot.cpe-iot.com:1883
    char uri[128] = {0};
    snprintf(uri, sizeof(uri), "mqtt://%s:%d", IOTM_SERVER_HOST, IOTM_SERVER_PORT);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = uri,
        .session.keepalive = IOTM_KEEP_ALIVE,
        .session.disable_clean_session = false,
    };

    // username/password（如果为空就不设置）
    if(strlen(IOTM_USERNAME) > 0){
        cfg.credentials.username = IOTM_USERNAME;
        cfg.credentials.authentication.password = IOTM_PASSWORD;
    }

    mqtt_set_last_will(&cfg);

    g_client = esp_mqtt_client_init(&cfg);
    if(!g_client){
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return;
    }

    esp_mqtt_client_register_event(g_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(g_client);
}

static void mqtt_manager_task(void *pvParameters)
{
    while (1) {
        // ---------- 1. 阻塞等待网络（永不超时）----------
        ESP_LOGI(TAG, "Waiting for 4G network...");
        // ec200x_wait_for_network(portMAX_DELAY);
        while(!ec200x_is_network_connected())
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        ESP_LOGI(TAG, "Network ready, starting MQTT resources...");

        // ---------- 2. 创建队列和 sysinfo 任务（若已清理则重建）----------
        if (sysinfo_queue == NULL) {
            sysinfo_queue = xQueueCreate(5, sizeof(uint32_t));
            xTaskCreate(sysinfo_task, "sysinfo_task", 1024 * 10,
                        NULL, 5, &sysinfo_task_handle);
        }
        if (ridinfo_queue == NULL) {
            ridinfo_queue = xQueueCreate(5, sizeof(uint32_t));
            xTaskCreate(ridinfo_task, "ridinfo_task", 1024 * 10,
                        NULL, 5, &ridinfo_task_handle);
        }

        // ---------- 3. 启动 MQTT 客户端（内部连接）----------
        mqtt_start();   // 设置 g_client 并连接，成功后自动订阅、创建定时器等

        // ---------- 4. 网络监控循环（每 5 秒检查一次）----------
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));

            if (!ec200x_is_network_connected()) {
                ESP_LOGW(TAG, "Network disconnected! Cleaning up...");

                // ----- 4.1 停止并销毁 MQTT 客户端 -----
                if (g_client) {
                    esp_mqtt_client_stop(g_client);
                    esp_mqtt_client_destroy(g_client);
                    g_client = NULL;
                }

                // ----- 4.2 删除定时器（在 MQTT_EVENT_CONNECTED 中创建）-----
                if (xTimer) {
                    xTimerStop(xTimer, 0);
                    xTimerDelete(xTimer, 0);
                    xTimer = NULL;
                }

                // ----- 4.3 删除 sysinfo 任务 -----
                if (sysinfo_task_handle) {
                    vTaskDelete(sysinfo_task_handle);
                    sysinfo_task_handle = NULL;
                }

                // ----- 4.4 删除队列 -----
                if (sysinfo_queue) {
                    vQueueDelete(sysinfo_queue);
                    sysinfo_queue = NULL;
                }

                ESP_LOGW(TAG, "Cleanup done. Re-entering wait for network...");
                break;  // 跳出内循环，重新等待网络
            }
        }
    }
}

void iot_init(void)
{
    // 防止重复初始化
    if (mqtt_manager_task_handle != NULL) {
        ESP_LOGW(TAG, "IoT already initialized, mqtt manager task already running");
        return;
    }

    // 创建 MQTT 生命周期管理任务（堆栈大小 8192）
    xTaskCreate(mqtt_manager_task, "mqtt_manager", 8192, NULL, 5, &mqtt_manager_task_handle);
    ESP_LOGI(TAG, "MQTT manager task started");
}

/**
 * @brief 释放所有 IoT 相关资源，停止所有任务
 * 
 * 调用此函数后，MQTT 客户端、定时器、sysinfo 任务及队列均被销毁，
 * MQTT manager 任务也将终止。函数可重复调用（幂等）。
 */
void free_iot_init(void)
{
    // ---------- 检查是否已初始化 ----------
    if (mqtt_manager_task_handle == NULL) {
        ESP_LOGW(TAG, "IoT not initialized or already freed, nothing to do");
        return;
    }

    ESP_LOGI(TAG, "Freeing IoT resources...");

    // ---------- 1. 先停止定时器，并等待可能的回调完成 ----------
    if (xTimer) {
        xTimerStop(xTimer, 0);               // 请求停止定时器
        vTaskDelay(pdMS_TO_TICKS(20));       // 给定时器任务足够时间退出回调
        xTimerDelete(xTimer, 0);             // 删除定时器
        xTimer = NULL;
        ESP_LOGI(TAG, "Timer stopped and deleted");
    }

    // ---------- 2. 停止并销毁 MQTT 客户端 ----------
    if (g_client) {
        esp_mqtt_client_stop(g_client);
        esp_mqtt_client_destroy(g_client);
        g_client = NULL;
        ESP_LOGI(TAG, "MQTT client destroyed");
    }

    // ---------- 3. 删除 sysinfo 任务（任务会自行退出，但直接删除更安全）----------
    if (sysinfo_task_handle) {
        vTaskDelete(sysinfo_task_handle);
        sysinfo_task_handle = NULL;
        ESP_LOGI(TAG, "Sysinfo task deleted");
    }

    // ---------- 4. 最后删除队列（此时已无任何发送者）----------
    if (sysinfo_queue) {
        vQueueDelete(sysinfo_queue);
        sysinfo_queue = NULL;
        ESP_LOGI(TAG, "Sysinfo queue deleted");
    }

    // ---------- 5. 删除 MQTT 管理任务本身 ----------
    if (mqtt_manager_task_handle) {
        vTaskDelete(mqtt_manager_task_handle);
        mqtt_manager_task_handle = NULL;
        ESP_LOGI(TAG, "MQTT manager task deleted");
    }

    ESP_LOGI(TAG, "All IoT resources freed.");
}