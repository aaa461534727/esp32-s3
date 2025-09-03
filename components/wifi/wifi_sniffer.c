#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "wifi_sniffer";

// WiFi信道配置（1-14）
static uint8_t channel = 6;
// 最大信道数
static uint8_t max_channel = 14;
// 信道切换间隔（毫秒）
static uint32_t channel_switch_interval = 1000;
// 定义队列大小和项目数
#define SNIFFER_QUEUE_LENGTH 200
#define SNIFFER_QUEUE_ITEM_SIZE sizeof(wifi_packet_info_t)

// WiFi帧类型
typedef enum {
    WIFI_FRAME_MGMT = 0x00,       // 管理帧
    WIFI_FRAME_CTRL = 0x01,       // 控制帧
    WIFI_FRAME_DATA = 0x02        // 数据帧
} wifi_frame_type;

// WiFi帧子类型（管理帧）
typedef enum {
    WIFI_FRAME_SUBTYPE_ASSOC_REQ = 0x00,
    WIFI_FRAME_SUBTYPE_ASSOC_RESP = 0x01,
    WIFI_FRAME_SUBTYPE_REASSOC_REQ = 0x02,
    WIFI_FRAME_SUBTYPE_REASSOC_RESP = 0x03,
    WIFI_FRAME_SUBTYPE_PROBE_REQ = 0x04,
    WIFI_FRAME_SUBTYPE_PROBE_RESP = 0x05,
    WIFI_FRAME_SUBTYPE_BEACON = 0x08,
    WIFI_FRAME_SUBTYPE_ATIM = 0x09,
    WIFI_FRAME_SUBTYPE_DISASSOC = 0x0A,
    WIFI_FRAME_SUBTYPE_AUTH = 0x0B,
    WIFI_FRAME_SUBTYPE_DEAUTH = 0x0C,
    WIFI_FRAME_SUBTYPE_ACTION = 0x0D  // 添加NAN帧类型
} wifi_mgmt_subtype;

// MAC地址结构
typedef struct {
    uint8_t addr[6];
} __attribute__((packed)) mac_addr;

// WiFi帧控制字段
typedef struct {
    uint16_t protocol:2;
    uint16_t type:2;
    uint16_t subtype:4;
    uint16_t to_ds:1;
    uint16_t from_ds:1;
    uint16_t more_frag:1;
    uint16_t retry:1;
    uint16_t pwr_mgmt:1;
    uint16_t more_data:1;
    uint16_t protected:1;
    uint16_t order:1;
} __attribute__((packed)) wifi_frame_control;

// WiFi帧头部（MAC头部）
typedef struct {
    wifi_frame_control fc;
    uint16_t duration;
    mac_addr addr1;
    mac_addr addr2;
    mac_addr addr3;
    uint16_t seq_ctrl;
} __attribute__((packed)) wifi_mac_hdr;

// 数据包信息结构体
typedef struct {
    int8_t rssi;
    uint8_t channel;
    uint8_t frame_type;
    uint8_t frame_subtype;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t frame_length;
    uint8_t payload[512];
} wifi_packet_info_t;

// 全局队列句柄
static QueueHandle_t wifi_packet_queue = NULL;

// 获取帧类型字符串
static const char* get_frame_type_str(uint8_t type) {
    switch (type) {
        case WIFI_FRAME_MGMT: return "Management";
        case WIFI_FRAME_CTRL: return "Control";
        case WIFI_FRAME_DATA: return "Data";
        default: return "Unknown";
    }
}

// 获取管理帧子类型字符串
static const char* get_mgmt_subtype_str(uint8_t subtype) {
    switch (subtype) {
        case WIFI_FRAME_SUBTYPE_BEACON: return "Beacon";
        case WIFI_FRAME_SUBTYPE_PROBE_REQ: return "Probe Request";
        case WIFI_FRAME_SUBTYPE_PROBE_RESP: return "Probe Response";
        case WIFI_FRAME_SUBTYPE_ASSOC_REQ: return "Association Request";
        case WIFI_FRAME_SUBTYPE_ASSOC_RESP: return "Association Response";
        case WIFI_FRAME_SUBTYPE_REASSOC_REQ: return "Reassociation Request";
        case WIFI_FRAME_SUBTYPE_REASSOC_RESP: return "Reassociation Response";
        case WIFI_FRAME_SUBTYPE_AUTH: return "Authentication";
        case WIFI_FRAME_SUBTYPE_DEAUTH: return "Deauthentication";
        case WIFI_FRAME_SUBTYPE_DISASSOC: return "Disassociation";
        case WIFI_FRAME_SUBTYPE_ATIM: return "ATIM";
        case WIFI_FRAME_SUBTYPE_ACTION: return "Action/NAN";  // 添加NAN帧处理
        default: return "Other";
    }
}

// MAC地址格式化输出
static void print_mac_addr(const char* prefix, const uint8_t* addr) {
    ESP_LOGI(TAG, "%s%02X:%02X:%02X:%02X:%02X:%02X", 
             prefix, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

// 打印数据包信息的任务
static void print_packet_task(void* arg) {
    wifi_packet_info_t packet_info;
    
    while (1) {
        // 等待队列中的数据包
        if (xQueueReceive(wifi_packet_queue, &packet_info, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "------------------------------");
            ESP_LOGI(TAG, "RSSI: %d", packet_info.rssi);
            ESP_LOGI(TAG, "Channel: %d", packet_info.channel);
            ESP_LOGI(TAG, "Frame Type: %s", get_frame_type_str(packet_info.frame_type));
            
            if (packet_info.frame_type == WIFI_FRAME_MGMT) {
                ESP_LOGI(TAG, "Subtype: %s", get_mgmt_subtype_str(packet_info.frame_subtype));
            }
            
            // 打印MAC地址信息
            print_mac_addr("Address 1: ", packet_info.addr1);
            print_mac_addr("Address 2: ", packet_info.addr2);
            print_mac_addr("Address 3: ", packet_info.addr3);
            
            // 打印帧长度
            ESP_LOGI(TAG, "Frame length: %d", packet_info.frame_length);
            
            // 打印帧内容（前64字节）
            uint16_t dump_len = (packet_info.frame_length > 64) ? 64 : packet_info.frame_length;
            ESP_LOG_BUFFER_HEXDUMP(TAG, packet_info.payload, dump_len, ESP_LOG_INFO);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // 避免任务饥饿
    }
}

// WiFi数据包捕获回调函数
static void wifi_sniffer_packet_handler(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    int beacon_offset=36;
    int nan_offset=24;
    int length=0;
    int i =0;
    // 检查数据包长度是否足够包含MAC头部
    if (pkt->rx_ctrl.sig_len < sizeof(wifi_mac_hdr)) {
        return;
    }
    
    wifi_mac_hdr* hdr = (wifi_mac_hdr*)pkt->payload;
    
    // 只处理管理帧
    if (hdr->fc.type != WIFI_FRAME_MGMT) {
        return;
    }

    // 准备数据包信息
    wifi_packet_info_t packet_info;
    packet_info.rssi = pkt->rx_ctrl.rssi;
    packet_info.channel = channel;
    packet_info.frame_type = hdr->fc.type;
    packet_info.frame_subtype = hdr->fc.subtype;
    packet_info.frame_length = pkt->rx_ctrl.sig_len;
    
    // 复制MAC地址
    memcpy(packet_info.addr1, hdr->addr1.addr, 6);
    memcpy(packet_info.addr2, hdr->addr2.addr, 6);
    memcpy(packet_info.addr3, hdr->addr3.addr, 6);
    
    // 检查Beacon帧
    if (hdr->fc.subtype == WIFI_FRAME_SUBTYPE_BEACON) {
        while(beacon_offset < pkt->rx_ctrl.sig_len){
            // 处理Beacon帧的内容
            if(pkt->payload[beacon_offset] == 0xDD){//vendor specific
				if(pkt->payload[beacon_offset+2]==0xFA && pkt->payload[beacon_offset+3]==0x0B && pkt->payload[beacon_offset+4]==0xBC && pkt->payload[beacon_offset+5] == 0x0D){//Remote ID
					length=pkt->payload[beacon_offset+1];
                    //ESP_LOG_BUFFER_HEXDUMP(TAG, &pkt->payload[beacon_offset], length, ESP_LOG_INFO);
                    memcpy(packet_info.payload, &pkt->payload[beacon_offset], length);
                    if(xQueueSend(wifi_packet_queue, &packet_info, 0) != pdTRUE) {
                        ESP_LOGE(TAG, "Failed to send packet to queue");
                    }
                }
            }
            beacon_offset++;
        }
    }
    // 检查NAN帧 (子类型13)
    else if (hdr->fc.subtype == 0x0D) {
        while(nan_offset < pkt->rx_ctrl.sig_len){
            // 处理NAN帧的内容
			if(pkt->payload[nan_offset] == 0x04 && pkt->payload[nan_offset+1] == 0x09){//Public Action
				if(pkt->payload[nan_offset+2]==0x50 && pkt->payload[nan_offset+3]==0x6F && pkt->payload[nan_offset+4]==0x9A && pkt->payload[nan_offset+5] == 0x13){
					length=pkt->payload[nan_offset+18]+19;
                    //ESP_LOG_BUFFER_HEXDUMP(TAG, &pkt->payload[nan_offset], length, ESP_LOG_INFO);
                    memcpy(packet_info.payload, &pkt->payload[nan_offset], length);
                    if(xQueueSend(wifi_packet_queue, &packet_info, 0) != pdTRUE) {
                        ESP_LOGE(TAG, "Failed to send packet to queue");
                    }
                }
            }
            nan_offset++;
        }
    }
    else
    {
        return;
    }
}

// 信道切换任务
static void channel_switch_task(void* arg) {
    while (1) {
        vTaskDelay(channel_switch_interval / portTICK_PERIOD_MS);
        channel = (channel % max_channel) + 1;
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        ESP_LOGI(TAG, "Switched to channel %d", channel);
    }
}

void wifi_sniffer_init(void) {
    // 初始化底层TCP/IP栈和事件循环
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 创建队列
    wifi_packet_queue = xQueueCreate(SNIFFER_QUEUE_LENGTH, SNIFFER_QUEUE_ITEM_SIZE);
    if (wifi_packet_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }
    
    // WiFi初始化配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // 设置WiFi为混杂模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // 设置混杂模式参数
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
    
    // 设置初始信道
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
    
    ESP_LOGI(TAG, "WiFi sniffer started");

    // 创建信道切换任务
    //xTaskCreate(channel_switch_task, "channel_switch", 2048, NULL, 5, NULL);
    
    // 创建数据包打印任务
    xTaskCreate(print_packet_task, "print_packet", 4096, NULL, 5, NULL);
}
