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

/* 存储捕获的数据包 */
typedef struct {
    uint8_t data[512];
    uint16_t length;
    int8_t rssi;
    uint32_t timestamp;
    uint8_t primary_phy;        /* 1=1M, 2=2M, 3=CODED */
    uint8_t secondary_phy;
    uint8_t sid;
    uint8_t adv_addr[6];
} ble_packet_t;

QueueHandle_t packet_queue;

/* RID 服务 UUID (小端): 0xFFFA */
#define RID_UUID_HIGH  0xFF
#define RID_UUID_LOW   0xFA

/* 检查 BLE AD 数据中是否包含 RID 服务 0xFAFF */
static bool is_rid_packet(const uint8_t *data, uint16_t len)
{
    int idx = 0;
    while (idx + 1 < len) {
        uint8_t ad_len = data[idx];
        if (ad_len == 0) break;
        idx++;
        if (idx + ad_len > len) break;
        uint8_t ad_type = data[idx];
        idx++;

        /* Service Data - 16-bit UUID */
        if (ad_type == 0x16 && ad_len >= 3) {
            if (data[idx] == RID_UUID_LOW && data[idx+1] == RID_UUID_HIGH)
                return true;
        }
        /* Manufacturer Specific Data */
        if (ad_type == 0xFF && ad_len >= 2) {
            if (data[idx] == RID_UUID_LOW && data[idx+1] == RID_UUID_HIGH)
                return true;
        }
        idx += (ad_len - 1);
    }
    return false;
}

/* 打印 GB 46750 TLV */
static void dump_gb_tlv(const uint8_t *data, int offset, int len)
{
    if (offset + 3 > len) return;
    if (data[offset] != 0xFF) return;

    printf("    [GB46750 TLV]\n");
    printf("    header: FF %02X %02X (version data_len)\n", data[offset+1], data[offset+2]);

    int remaining = len - offset;
    printf("    raw: ");
    for (int i = 0; i < remaining && i < 80; i++)
        printf("%02X ", data[offset + i]);
    printf("\n");

    if (offset + 6 <= len) {
        printf("    bitmap[0]=0x%02X", data[offset+3]);
        if (offset + 7 <= len) printf(" [1]=0x%02X", data[offset+4]);
        if (offset + 8 <= len) printf(" [2]=0x%02X", data[offset+5]);
        printf("\n");
    }
}

/* 打印 AD structure 各元素 */
static void dump_ad_elements(const uint8_t *data, uint16_t len)
{
    int idx = 0;
    while (idx + 1 < len) {
        uint8_t ad_len = data[idx];
        if (ad_len == 0) { idx++; continue; }
        idx++;
        if (idx + ad_len > len) break;
        uint8_t ad_type = data[idx];

        const char *type_name = "?";
        switch (ad_type) {
            case 0x01: type_name = "Flags"; break;
            case 0x08: type_name = "ShortName"; break;
            case 0x09: type_name = "FullName"; break;
            case 0x16: type_name = "Service16"; break;
            case 0xFF: type_name = "ManuSpecific"; break;
        }
        printf("    AD[len=%d type=0x%02X %s]: ", ad_len, ad_type, type_name);
        for (int j = 0; j < ad_len && j < 20; j++)
            printf("%02X ", data[idx + j]);
        if (ad_len > 20) printf("...");
        printf("\n");

        /* 检测 RID UUID */
        if ((ad_type == 0x16 || ad_type == 0xFF) && ad_len >= 3 &&
            data[idx] == RID_UUID_LOW && data[idx+1] == RID_UUID_HIGH) {

            /* AD 类型 0x16 的 ODID subtype 在 idx+2 */
            if (ad_type == 0x16) {
                if (ad_len >= 4 && data[idx+2] == 0x0D) {
                    printf("      -> ASTM F3411 (旧国标) counter=%d\n", data[idx+3]);
                    /* data from idx+4 onwards */
                    dump_gb_tlv(data, idx+3, ad_len-1);
                } else {
                    printf("      -> RID Service Data (ad_len=%d)\n", ad_len);
                    /* 检查 idx+2 是否 0xFF (GB TLV 直接跟在 UUID 后) */
                    dump_gb_tlv(data, idx+2, ad_len);
                }
            }
            /* AD 类型 0xFF 的 manufacturer data, idx+2 开始可能是 GB TLV */
            if (ad_type == 0xFF) {
                printf("      -> RID Manufacturer Data\n");
                dump_gb_tlv(data, idx+2, ad_len);
            }
        }

        idx += (ad_len - 1);
    }
}

/* 打印一个包 */
static void print_packet(const ble_packet_t *pkt)
{
    printf("\n======== [BLE Packet] ========\n");
    printf("  RSSI: %d dBm\n", pkt->rssi);

    /* PHY */
    printf("  PHY: ");
    switch (pkt->primary_phy) {
        case 1: printf("1M"); break;
        case 2: printf("2M"); break;
        case 3: printf("CODED"); break;
        default: printf("PHY_%d", pkt->primary_phy);
    }
    if (pkt->secondary_phy != pkt->primary_phy && pkt->secondary_phy != 0) {
        printf(" -> ");
        switch (pkt->secondary_phy) {
            case 1: printf("1M"); break;
            case 2: printf("2M"); break;
            case 3: printf("CODED"); break;
            default: printf("PHY_%d", pkt->secondary_phy);
        }
    }

    printf("  SID: %d", pkt->sid);
    printf("  MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           pkt->adv_addr[5], pkt->adv_addr[4], pkt->adv_addr[3],
           pkt->adv_addr[2], pkt->adv_addr[1], pkt->adv_addr[0]);
    printf("  Total: %d bytes\n", pkt->length);

    /* 打印 AD 结构 */
    dump_ad_elements(pkt->data, pkt->length);

    /* 完整 hex dump */
    printf("  Full hex:\n");
    for (int i = 0; i < pkt->length; i += 16) {
        int chunk = (pkt->length - i) > 16 ? 16 : pkt->length - i;
        printf("    %04X: ", i);
        for (int j = 0; j < chunk; j++) printf("%02X ", pkt->data[i + j]);
        for (int j = chunk; j < 16; j++) printf("   ");
        printf(" ");
        for (int j = 0; j < chunk; j++) {
            uint8_t c = pkt->data[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        printf("\n");
    }
    printf("\n");
}

/* ========== BLE 5.0 Extended Scan 回调 ========== */

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_EXT_ADV_REPORT_EVT:
    {
        esp_ble_gap_ext_adv_report_t *report = &param->ext_adv_report.params;

        uint16_t adv_data_len = report->adv_data_len;
        if (adv_data_len < 5) break;

        /* 过滤: 只保留 RID 包 (含 0xFAFF) */
        if (!is_rid_packet(report->adv_data, adv_data_len))
            break;

        ble_packet_t packet;
        memset(&packet, 0, sizeof(packet));
        packet.timestamp = esp_log_timestamp();
        packet.rssi = report->rssi;
        packet.primary_phy = report->primary_phy;
        packet.secondary_phy = report->secondly_phy;
        packet.sid = report->sid;
        memcpy(packet.adv_addr, report->addr, 6);

        packet.length = (adv_data_len > sizeof(packet.data)) ? sizeof(packet.data) : adv_data_len;
        memcpy(packet.data, report->adv_data, packet.length);

        if (packet_queue != NULL) {
            if (uxQueueMessagesWaiting(packet_queue) >= 50) {
                ble_packet_t dummy;
                xQueueReceive(packet_queue, &dummy, 0);
            }
            xQueueSendToBack(packet_queue, &packet, 0);
        }
        break;
    }

    case ESP_GAP_BLE_SET_EXT_SCAN_PARAMS_COMPLETE_EVT:
        ESP_LOGI(TAG, "Ext scan params set, status=%d", param->set_ext_scan_params.status);
        break;

    case ESP_GAP_BLE_EXT_SCAN_START_COMPLETE_EVT:
        if (param->ext_scan_start.status != ESP_BT_STATUS_SUCCESS)
            ESP_LOGE(TAG, "Ext scan start failed: %d", param->ext_scan_start.status);
        else
            ESP_LOGI(TAG, "Ext scan started successfully");
        break;

    case ESP_GAP_BLE_EXT_SCAN_STOP_COMPLETE_EVT:
        ESP_LOGI(TAG, "Ext scan stopped, status=%d", param->ext_scan_stop.status);
        break;

    default:
        break;
    }
}

/* ========== BLE 初始化 ========== */

static void ble_init(void)
{
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return;
    }

    /* BLE 5.0 Extended Scan 参数 */
    esp_ble_ext_scan_params_t ext_scan_params = {
        .own_addr_type = BLE_ADDR_TYPE_RANDOM,
        .filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
        .cfg_mask = ESP_BLE_GAP_EXT_SCAN_CFG_UNCODE_MASK | ESP_BLE_GAP_EXT_SCAN_CFG_CODE_MASK,
        .uncoded_cfg = {
            .scan_type = BLE_SCAN_TYPE_PASSIVE,
            .scan_interval = 0x100,    /* 256 × 0.625ms = 160ms */
            .scan_window = 0x50,       /* 80 × 0.625ms = 50ms */
        },
        .coded_cfg = {
            .scan_type = BLE_SCAN_TYPE_PASSIVE,
            .scan_interval = 0x100,
            .scan_window = 0x50,
        },
    };

    ret = esp_ble_gap_set_ext_scan_params(&ext_scan_params);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Set ext scan params failed: %s", esp_err_to_name(ret));
        return;
    }
}

static void start_ext_scan(void)
{
    esp_err_t ret = esp_ble_gap_start_ext_scan(0, 0);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "Start ext scan failed: %s", esp_err_to_name(ret));
}

/* ========== 处理任务 ========== */

void packet_handler_task(void *pvParameters)
{
    ble_packet_t packet;
    while (1) {
        if (xQueueReceive(packet_queue, &packet, portMAX_DELAY) == pdPASS) {
            print_packet(&packet);
        }
    }
}

/* ========== 公开 API ========== */

void ble_sniffer_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE Sniffer (Extended Scan)...");

    packet_queue = xQueueCreate(50, sizeof(ble_packet_t));
    if (packet_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create packet queue");
        return;
    }

    ble_init();

    xTaskCreate(packet_handler_task, "packet_handler", 4096, NULL, 5, NULL);

    vTaskDelay(pdMS_TO_TICKS(1500));
    start_ext_scan();

    ESP_LOGI(TAG, "BLE Sniffer started! Scanning all PHYs (1M, 2M, CODED).");
    ESP_LOGI(TAG, "Filtering for RID packets (UUID 0xFAFF).");
    ESP_LOGI(TAG, "Printing both old (ASTM) and new (GB46750) RID data.");
}
