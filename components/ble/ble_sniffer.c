#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_system.h"

static const char *TAG = "BLE_SNIFFER";

// 存储捕获的数据包
typedef struct {
    uint8_t data[ESP_BLE_ADV_DATA_LEN_MAX + ESP_BLE_SCAN_RSP_DATA_LEN_MAX];
    uint16_t length;
    int8_t rssi;
    uint32_t timestamp;
    esp_ble_evt_type_t evt_type;
} ble_packet_t;

// 数据包队列
QueueHandle_t packet_queue;

// BLE抓包回调函数
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = param;
            switch (scan_result->scan_rst.search_evt) {
                case ESP_GAP_SEARCH_INQ_RES_EVT: {
                    // 检查数据包长度是否符合要求
                    uint16_t adv_len = scan_result->scan_rst.adv_data_len;
                    uint16_t rsp_len = scan_result->scan_rst.scan_rsp_len;
                    uint16_t total_len = adv_len + rsp_len;
                    
                    // 至少需要5字节才能检查前5个字节
                    if (total_len < 6) {
                        break;
                    }
                    
                    // 检查前5个字节是否符合过滤条件
                    if (scan_result->scan_rst.ble_adv[0] == 0x1E &&
                        scan_result->scan_rst.ble_adv[1] == 0x16 &&
                        scan_result->scan_rst.ble_adv[2] == 0xFA &&
                        scan_result->scan_rst.ble_adv[3] == 0xFF &&
                        scan_result->scan_rst.ble_adv[4] == 0x0D) {
                        
                        // 创建数据包结构
                        ble_packet_t packet;
                        packet.timestamp = esp_log_timestamp();
                        packet.rssi = scan_result->scan_rst.rssi;
                        packet.evt_type = scan_result->scan_rst.ble_evt_type;
                        
                        // 确保不超过缓冲区大小
                        packet.length = (total_len > sizeof(packet.data)) ? sizeof(packet.data) : total_len;
                        
                        // 复制整个数据块
                        memcpy(packet.data, scan_result->scan_rst.ble_adv, packet.length);
                        
                        // 将匹配的数据包发送到队列
                        if (packet_queue != NULL) {
                            // 如果队列已满，丢弃最旧的数据包
                            if (uxQueueMessagesWaiting(packet_queue) >= 100) {
                                ble_packet_t dummy;
                                xQueueReceive(packet_queue, &dummy, 0);
                            }
                            xQueueSendToBack(packet_queue, &packet, 0);
                        }
                    }
                    break;
                }
                case ESP_GAP_SEARCH_INQ_CMPL_EVT:
                    ESP_LOGI(TAG, "Inquiry complete");
                    break;
                    
                default:
                    break;
            }
            break;
        }
        
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "Scan parameters set");
            break;
            
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Scan start failed: %d", param->scan_start_cmpl.status);
            } else {
                ESP_LOGI(TAG, "Scan started successfully");
            }
            break;
            
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Scan stop failed: %d", param->scan_stop_cmpl.status);
            } else {
                ESP_LOGI(TAG, "Scan stopped successfully");
            }
            break;
            
        default:
            break;
    }
}

// 初始化BLE控制器
void ble_init(void)
{
    // 初始化蓝牙控制器
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth Controller initialize failed: %s", esp_err_to_name(ret));
        return;
    }

    // 启用蓝牙控制器
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluetooth Controller enable failed: %s", esp_err_to_name(ret));
        return;
    }

    // 初始化Bluedroid栈
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid initialize failed: %s", esp_err_to_name(ret));
        return;
    }

    // 启用Bluedroid栈
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return;
    }

    // 注册GAP回调函数
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return;
    }

    // 设置扫描参数 - 支持BLE 4.0和5.0
    esp_ble_scan_params_t ble_scan_params = {
        .scan_type = BLE_SCAN_TYPE_PASSIVE,        // 被动扫描
        .own_addr_type = BLE_ADDR_TYPE_RANDOM,     // 使用随机地址保护隐私
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval = 0x100,                    // 扫描间隔 (256 * 0.625ms = 160ms)
        .scan_window = 0x50,                       // 扫描窗口 (80 * 0.625ms = 50ms)
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE // 禁用重复过滤
    };
    
    // 设置扫描参数
    ret = esp_ble_gap_set_scan_params(&ble_scan_params);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Set scan parameters failed: %s", esp_err_to_name(ret));
        return;
    }
}

// 开始BLE扫描
void start_ble_scan(void)
{
    esp_err_t ret = esp_ble_gap_start_scanning(0);  // 0表示无限扫描
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Start scanning failed: %s", esp_err_to_name(ret));
    }
}

// 获取事件类型字符串
const char* get_evt_type_string(esp_ble_evt_type_t evt_type) {
    switch (evt_type) {
        case ESP_BLE_EVT_CONN_ADV: return "CONNECTABLE_ADV";
        case ESP_BLE_EVT_CONN_DIR_ADV: return "DIRECTED_ADV";
        case ESP_BLE_EVT_DISC_ADV: return "SCANNABLE_ADV";
        case ESP_BLE_EVT_NON_CONN_ADV: return "NON_CONNECTABLE_ADV";
        case ESP_BLE_EVT_SCAN_RSP: return "SCAN_RESPONSE";
        default: return "UNKNOWN";
    }
}

// 打印数据包内容（原始数据和ASCII）
void print_packet(ble_packet_t *packet)
{
    // 打印基本信息
    printf("\n[Filtered BLE Packet] Timestamp: %ld, RSSI: %ddB, Event: %s, Length: %d\n", 
           packet->timestamp, packet->rssi, 
           get_evt_type_string(packet->evt_type),
           packet->length);
    
    // 打印原始数据（十六进制格式）
    printf("Raw Data:\n");
    for (int i = 0; i < packet->length; i += 16) {
        uint16_t chunk_size = (packet->length - i) > 16 ? 16 : packet->length - i;
        printf("%04X: ", i);
        
        // 打印十六进制
        for (int j = 0; j < chunk_size; j++) {
            printf("%02X ", packet->data[i + j]);
        }
        
        // 对齐
        for (int j = chunk_size; j < 16; j++) {
            printf("   ");
        }
        
        printf(" ");
        
        // 打印ASCII
        for (int j = 0; j < chunk_size; j++) {
            uint8_t c = packet->data[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        
        printf("\n");
    }
    printf("\n");
}

// 数据包处理任务
void packet_handler_task(void *pvParameters)
{
    ble_packet_t packet;
    
    while (1) {
        if (xQueueReceive(packet_queue, &packet, portMAX_DELAY) == pdPASS) {
            // 打印捕获的数据包
            print_packet(&packet);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ble_sniffer_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE Sniffer...");
    
    // 创建数据包队列
    packet_queue = xQueueCreate(100, sizeof(ble_packet_t));
    if (packet_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create packet queue");
        return;
    }
    
    // 初始化BLE抓包器
    ble_init();
    
    // 创建数据包处理任务
    xTaskCreate(packet_handler_task, "packet_handler", 4096, NULL, 5, NULL);
    
    // 等待一段时间后开始扫描
    vTaskDelay(pdMS_TO_TICKS(1000));
    start_ble_scan();
    
    ESP_LOGI(TAG, "BLE Sniffer started. Capturing BLE packets...");
    ESP_LOGI(TAG, "Listening for BLE 4.0 and 5.0 packets...");
    

}
