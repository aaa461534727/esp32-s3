#include "rid_wifi_sniffer.h"

static const char *TAG = "rid_wifi_sniffer";
//添加初始化状态标志
static bool wifi_sniffer_initialized = false;
TaskHandle_t send_rid_task_handle = NULL;
static esp_netif_t *s_ap_netif = NULL;   // 保存AP网络接口句柄
static SemaphoreHandle_t rid_mutex = NULL;  //FreeRTOS互斥量
float loss_rate_value = 0.0f;
int test_mode = 0;
int max_rssi = -127;
static char g_last_rid_data[DATA_BUF_LEN] = {0};  // 保存最后一次的无人机数据JSON
static volatile uint32_t s_mgmt_frame_count = 0;          // 管理帧计数器
static portMUX_TYPE s_count_mutex = portMUX_INITIALIZER_UNLOCKED;  // 保护计数器的自旋锁
static esp_timer_handle_t s_count_timer = NULL;           // 定时器句柄

// 定义无线链表头节点
Node *wifi_head = NULL;
// 定义设备信息变量并初始化
struct device_Info device_Info = {
    .status = false,
    .id = {0},
    .rid_port = 0,
    .rid_server = {0},
    .dev_lat = 0.0,
    .dev_lon = 0.0,
    .model = {0},
    .device_id = {0}
};

// 创建链表节点
Node *createNode(rid_info_t data)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        ESP_LOGI(TAG,"Error: unable to create new node.\n");
        exit(1);
    }
    newNode->data = data;
    newNode->next = NULL; // 初始化下一个节点指针为NULL
    return newNode;
}

// 添加或更新节点
void addOrUpdateNode(Node **head, rid_info_t data)
{
    Node *temp = *head;
    Node *prev = NULL;
    int found = 0;
    int i = 0;
    // 遍历链表查找是否有重复节点
    while (temp != NULL) {
        if ((strlen(data.sn) && strcmp(temp->data.sn, data.sn) == 0) || (strlen(data.uas_id) && strcmp(temp->data.uas_id, data.uas_id) == 0) ||
            (strlen(data.uuid) && strcmp(temp->data.uuid, data.uuid) == 0) || (strlen(data.seid) && strcmp(temp->data.seid, data.seid) == 0)) {
            found = 1;
            // 更新节点数据
            // 在判断具体是哪个消息
            // 0x0
            if (strlen(data.sn))
                strcpy(temp->data.sn, data.sn);
            if (strlen(data.uas_id))
                strcpy(temp->data.uas_id, data.uas_id);
            if (strlen(data.uuid))
                strcpy(temp->data.uuid, data.uuid);
            if (strlen(data.seid))
                strcpy(temp->data.seid, data.seid);
            // 0x1
            temp->data.status = data.status;
            temp->data.direction = data.direction;
            temp->data.lon = data.lon;
            temp->data.lat = data.lat;
            temp->data.alt = data.alt;
            temp->data.speed = data.speed;
            temp->data.v_speed = data.v_speed;
            temp->data.air_high = data.air_high;
            temp->data.geo_high = data.geo_high;
            temp->data.high_type = data.high_type;
            temp->data.ew_flag = data.ew_flag;
            temp->data.g_high = data.g_high;
            temp->data.hor_accuracy = data.hor_accuracy;
            temp->data.ver_accuracy = data.ver_accuracy;
            temp->data.alt_accuracy = data.alt_accuracy;
            temp->data.speed_accuracy = data.speed_accuracy;
            temp->data.timestamp = data.timestamp;
            temp->data.time_acc = data.time_acc;
            // 0x2
            temp->data.authtype = data.authtype;
            memcpy(temp->data.authentication_data, data.authentication_data, sizeof(data.authentication_data));
            memcpy(temp->data.aut_timestamp, data.aut_timestamp, sizeof(data.aut_timestamp));
            // 0x3
            temp->data.selfid_type = data.selfid_type;
            memcpy(temp->data.selfid, data.selfid, sizeof(data.selfid));
            // 0x4
            temp->data.zone = data.zone;
            temp->data.operator_type = data.operator_type;
            temp->data.distance = data.distance;
            temp->data.ilotLat = data.ilotLat;
            temp->data.ilotLon = data.ilotLon;
            temp->data.areacount = data.areacount;
            temp->data.area_ceiling = data.area_ceiling;
            temp->data.area_floor = data.area_floor;
            temp->data.category = data.category;
            temp->data.class = data.class;
            temp->data.operator_altitude = data.operator_altitude;
            memcpy(temp->data.sys_timestamp, data.sys_timestamp, sizeof(data.sys_timestamp));
            // 0x5
            temp->data.operator_id_type = data.operator_id_type;
            memcpy(temp->data.operator_id, data.operator_id, sizeof(data.operator_id));
            if (0)
                ESP_LOGI(TAG,"-------------node is already exit-----------\n");
            break;
        }
        prev = temp;
        temp = temp->next;
    }
    // 如果没有找到重复节点，则添加新节点
    if (!found) {
        Node *newNode = createNode(data);
        if (prev == NULL) {
            // 链表为空或新节点应添加到头部
            newNode->next = *head;
            *head = newNode;
        } else {
            // 新节点添加到链表中间或尾部
            prev->next = newNode;
        }
    }
}

// 打印链表数据
void printLinkedList(Node *wifi_head)
{
    int i;
    ESP_LOGI(TAG,"list start\n");
    struct Node *current = wifi_head;
    while (current != NULL) {
        ESP_LOGI(TAG,"\n lon=%.7f lat=%.7f alt=%.1f operator_altitude=%.1f speed=%.1f dir=%d distance=%.1f ilotLat=%.7f ilotLon=%.7f timestemp=%u sn=%s uasid= %s uuid = %s seid =%s\n",
               current->data.lon, current->data.lat, current->data.alt, current->data.operator_altitude, current->data.speed, current->data.direction, current->data.distance, current->data.ilotLat, current->data.ilotLon,
               current->data.timestamp, current->data.sn, current->data.uas_id, current->data.uuid, current->data.seid);

        current = current->next;
    }
    ESP_LOGI(TAG,"\nnull\n");
}

// 检查链表里面是否有节点
int checkLinkedList()
{
    if (wifi_head == NULL)
        return 0;

    struct Node *temp = wifi_head;
    if (temp != NULL)
        return 1;
    else
        return 0;
}

// 释放链表内存
void free_list(Node **wifi_head)
{
    Node *current = *wifi_head;
    Node *next = NULL;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *wifi_head = NULL;
}

char *get_model_from_sn(const char *uas_id, const char *sn)
{
    const char *search_id = sn && sn[0] ? sn : (uas_id && uas_id[0] ? uas_id : NULL);
    if (!search_id) return NULL;
    
    FILE *file = fopen(RID_SN_LIST_FILE, "r");
    if (!file) return NULL;
    
    char *model = NULL;
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *json_data = malloc(file_size + 1);
    if (!json_data) {
        fclose(file);
        return NULL;
    }
    
    fread(json_data, 1, file_size, file);
    json_data[file_size] = '\0';
    fclose(file);
    
    cJSON *root = cJSON_Parse(json_data);
    if (root) {
        cJSON *groups = cJSON_GetObjectItem(root, "groups");
        if (groups) {
            // 确定目标分组
            const char *target_group = NULL;
            if (strncmp(search_id, "1581", 4) == 0) {
                target_group = "DJI";  // DJI设备
            } else {
                target_group = "Test";  // 其他设备归入测试组
            }
            
            // 在目标分组中搜索
            cJSON *group = cJSON_GetObjectItem(groups, target_group);
            if (group) {
                // 根据ID前缀确定目标类别
                const char *target_category = NULL;
                if (strlen(search_id) > 5) {
                    if (search_id[4] == 'F') {
                        target_category = "consumer";  // 消费级设备
                    } else if (search_id[4] == 'A') {
                        target_category = "Industrial";  // 工业级设备
                    }
                }
                
                // 根据ID前缀确定目标类别
                if (target_category) {
                    cJSON *category = cJSON_GetObjectItem(group, target_category);
                    if (category) {
                        cJSON *device = category->child;
                        while (device) {
                            // 跳过空值映射
                            if (!device->valuestring || strlen(device->valuestring) == 0) {
                                device = device->next;
                                continue;
                            }
                            
                            const char *prefix = device->string;
                            size_t prefix_len = strlen(prefix);
                            
                            // 前缀匹配
                            if (strncmp(search_id, prefix, prefix_len) == 0) {
                                model = strdup(device->valuestring);
                                goto found;
                            }
                            device = device->next;
                        }
                    }
                }
                
                //  目标类别未找到，遍历分组内所有类别
                cJSON *category = group->child;
                while (category) {
                    // 跳过已搜索的目标类别
                    if (target_category && strcmp(category->string, target_category) == 0) {
                        category = category->next;
                        continue;
                    }
                    
                    cJSON *device = category->child;
                    while (device) {
                        // 跳过空值映射
                        if (!device->valuestring || strlen(device->valuestring) == 0) {
                            device = device->next;
                            continue;
                        }
                        
                        const char *prefix = device->string;
                        size_t prefix_len = strlen(prefix);
                        
                        // 前缀匹配
                        if (strncmp(search_id, prefix, prefix_len) == 0) {
                            model = strdup(device->valuestring);
                            goto found;
                        }
                        device = device->next;
                    }
                    category = category->next;
                }
            }
            
            //  目标分组未找到，全局搜索其他分组
            if (!model) {
                cJSON *other_group = groups->child;
                while (other_group) {
                    // 跳过已搜索的目标分组
                    if (strcmp(other_group->string, target_group) == 0) {
                        other_group = other_group->next;
                        continue;
                    }
                    
                    // 遍历分组内所有类别
                    cJSON *category = other_group->child;
                    while (category) {
                        cJSON *device = category->child;
                        while (device) {
                            // 跳过空值映射
                            if (!device->valuestring || strlen(device->valuestring) == 0) {
                                device = device->next;
                                continue;
                            }
                            
                            const char *prefix = device->string;
                            size_t prefix_len = strlen(prefix);
                            
                            // 前缀匹配
                            if (strncmp(search_id, prefix, prefix_len) == 0) {
                                model = strdup(device->valuestring);
                                goto found;
                            }
                            device = device->next;
                        }
                        category = category->next;
                    }
                    other_group = other_group->next;
                }
            }
        found: ; // 标签位置
        }
        cJSON_Delete(root);
    }
    free(json_data);
    return model;
}

//提取rid数据
void get_data_from_index(unsigned char *data, int data_size, int start_index, int len, unsigned char *buffer) {
    if (start_index < 0 || start_index >= data_size) {
        printf("Error: start_index out of bounds\n");
        return;
    }
    if (start_index + len > data_size) {
        printf("Error: len is too large, exceeds array bounds\n");
        return;
    }

    // 使用memcpy从data数组复制数据到buffer
    memcpy(buffer, data + start_index, len);
}
// data: [DD][21][FA][0B][BC][0D][B7][F2][19][01][02][11][32][30][35][31][46][45][41][42][50][54][30][30][30][30][30][30][30][31][33][35][00]
void recv_parse_data(unsigned char *data, int8_t rssi,int channel,int channel_busy_time,int loss_rate)
{
    int i = 0, cnt = 0, offset = 0, sub_num = 0, data_err = 0;
    // printf("start");
    if ((data[0] == 0XDD && data[2] == 0xFA && data[3] == 0x0B && data[4] == 0xBC && data[5] == 0x0D) ||
        (data[0] == 0x04 && data[1] == 0x09 && data[2] == 0x50 && data[3] == 0x6F)) {
        if (data[0] == 0XDD) {
            offset = 6; // Beacon跳过标识字节
            // printf("Beacon\r\n");
        } else {
            offset = 19; // NaN跳过标识字节
            // printf("NaN\r\n");
        }

        offset++; // 跳过消息计数字节

        if ((data[offset] >> 4) != 0xF) // data[7] 高四位固定值：0xF，低四位为协议版本号
            return;

        offset++;

        if (data[offset] != PACKAGE_LENGTH) // 每条报文长度限制为25
            return;

        offset++;

        sub_num = data[offset]; // 报文内容中包含的报文数量，最大值为10
        offset++;

        rid_info_t info = {0};
		info.rssi = rssi;
        info.channel = channel;
		info.channel_busy_time = channel_busy_time;
		loss_rate_value = (float)loss_rate / 100.0f;

        if (test_mode) {
            printf("\n----sum_msg : %d--------\n[", sub_num);
            for (i = 0; i < sub_num * 25; i++) {
                cnt++;
                printf("%02X ", data[offset + i]);
                if (cnt == 25) {
                    printf("]\n[");
                    cnt = 0;
                }
            }
            printf("\n");
        }
        for (i = 0; i < sub_num; i++) {
            switch ((data[offset] >> 4)) {
            case BASID_ID:
                offset = rid_parse_basic_id(data, offset, &info);
                break;
            case LOCATION_VECTOR:
                offset = rid_parse_location_vector(data, offset, &info);
                break;
            case RESERV:
                offset = rid_parse_reserve(data, offset, &info);
                break;
            case RUN_COMMENT:
                offset = rid_parse_run_comment(data, offset, &info);
                break;
            case SYSTEM:
                offset = rid_parse_system_info(data, offset, &info);
                break;
            case OPERATOR_ID:
                offset = rid_parse_operator_id(data, offset, &info);
                break;
            default:
                printf("remote ID Data Error!\n");
                data_err = 1;
                break;
            }
            if (data_err)
            {
                memset(&info, 0, sizeof(rid_info_t));
                goto end;           
            }
        }

        if (!data_err) {
            if (rid_mutex != NULL) {
                if (xSemaphoreTake(rid_mutex, portMAX_DELAY) == pdTRUE) {
                    addOrUpdateNode(&wifi_head, info);
                    // print_list_data();
                    xSemaphoreGive(rid_mutex);
                }
            } else {
                addOrUpdateNode(&wifi_head, info);
            }
        }
    }

end:
    return;
}

int prepare_udp_send(char *buf)
{
    char *data = NULL;
    char tmp_buf[16] = {0};
    int i = 0;
    if (rid_mutex != NULL) {
        if (xSemaphoreTake(rid_mutex, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to take mutex");
            return 0;
        }
    }
    int data_exist = 0;
    cJSON *root, *droneArry;

    root = cJSON_CreateObject();
    droneArry = cJSON_CreateArray();

    cJSON_AddItemToObject(root, "drones", droneArry);
    struct Node *temp = wifi_head;
    if (test_mode)
        printLinkedList(wifi_head);
    while (temp != NULL) {
        cJSON *obj = cJSON_CreateObject();
    // 0x1
        cJSON_AddNumberToObject(obj, "status", temp->data.status);
        cJSON_AddNumberToObject(obj, "direction", temp->data.direction);
        cJSON_AddNumberToObject(obj, "lon", temp->data.lon);
        cJSON_AddNumberToObject(obj, "lat", temp->data.lat);
        cJSON_AddNumberToObject(obj, "alt", temp->data.alt);
        cJSON_AddNumberToObject(obj, "speed", temp->data.speed);
        cJSON_AddNumberToObject(obj, "v_speed", temp->data.v_speed);
        cJSON_AddNumberToObject(obj, "timestamp", temp->data.timestamp);
        cJSON_AddNumberToObject(obj, "time_acc", temp->data.time_acc);
        cJSON_AddNumberToObject(obj, "air_high", temp->data.air_high);
        cJSON_AddNumberToObject(obj, "geo_high", temp->data.geo_high);
        cJSON_AddNumberToObject(obj, "high_type", temp->data.high_type);
        cJSON_AddNumberToObject(obj, "g_high", temp->data.g_high);
        // 0x4
        if (temp->data.distance > 0) {
            cJSON_AddNumberToObject(obj, "distance", temp->data.distance);
        } else {
            memset(tmp_buf, 0, sizeof(tmp_buf));
            ble_get_distance(temp->data, tmp_buf);
            cJSON_AddNumberToObject(obj, "distance", atoi(tmp_buf));
        }
        cJSON_AddNumberToObject(obj, "ilotLon", temp->data.ilotLon);
        cJSON_AddNumberToObject(obj, "ilotLat", temp->data.ilotLat);
        // 0x0
        char *model = get_model_from_sn(temp->data.uas_id, temp->data.sn);
        cJSON_AddStringToObject(obj, "model", model ? model : "");
        if (model) free(model);
        cJSON_AddNumberToObject(obj, "ua_type", temp->data.ua_type);
        //cJSON_AddNumberToObject(obj, "id_type", temp->data.id_type);
        // cJSON_AddStringToObject(obj, "ua_type", temp->data->ua_type_str);
        // cJSON_AddStringToObject(obj, "id_type", temp->data->id_type_str);
        cJSON_AddStringToObject(obj, "id", temp->data.uas_id);
        cJSON_AddStringToObject(obj, "sn", temp->data.sn);
        //cJSON_AddStringToObject(obj, "sys_time", temp->data.sys_timestamp);
        // 0x2
        cJSON_AddNumberToObject(obj, "authtype", temp->data.authtype);
        cJSON_AddStringToObject(obj, "authentication_data", temp->data.authentication_data);
        //cJSON_AddStringToObject(obj, "aut_timestamp", temp->data.aut_timestamp);
        // 0x3
        cJSON_AddNumberToObject(obj, "selfid_type", temp->data.selfid_type);
        cJSON_AddStringToObject(obj, "selfid", temp->data.selfid);
        // 0x5
        cJSON_AddNumberToObject(obj, "operator_id_type", temp->data.operator_id_type);
        cJSON_AddStringToObject(obj, "operator_id", temp->data.operator_id);
        cJSON_AddNumberToObject(obj, "operator_altitude", temp->data.operator_altitude);

        if(temp->data.rssi > max_rssi)
            max_rssi = temp->data.rssi;
        cJSON_AddNumberToObject(obj, "rssi", temp->data.rssi);
        cJSON_AddNumberToObject(obj, "channel", temp->data.channel);
        cJSON_AddNumberToObject(obj, "channel_busy_time", temp->data.channel_busy_time);

        cJSON_AddItemToArray(droneArry, obj);

        data_exist = 1;
        temp = temp->next;
    }
    
    if (data_exist == 0)
        goto end;
    else {
        cJSON *dev_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(dev_obj, "id", device_Info.device_id);
        cJSON_AddBoolToObject(dev_obj, "status", true);
        cJSON_AddNumberToObject(dev_obj, "lon", device_Info.dev_lon);
        cJSON_AddNumberToObject(dev_obj, "lat", device_Info.dev_lat);
        cJSON_AddNumberToObject(dev_obj, "max_rssi", max_rssi);
        cJSON_AddItemToObject(root, "devices", dev_obj);
        cJSON_AddNumberToObject(dev_obj, "listen_type", 0); // 0-wifi;1-blue

        data = cJSON_Print(root);
        snprintf(buf, DATA_BUF_LEN, "%s", data);
        //将数据保存到全局缓冲区给iot
        strncpy(g_last_rid_data, buf, DATA_BUF_LEN - 1);
        g_last_rid_data[DATA_BUF_LEN - 1] = '\0';

        if (test_mode)
            printf("%s\r\n", buf);
    }
end:
    cJSON_Delete(root);

    if (data)
        free(data);

    // 清空链表
    free_list(&wifi_head);

    // 释放FreeRTOS互斥量
    if (rid_mutex != NULL) {
        xSemaphoreGive(rid_mutex);
    }

    return data_exist;
}

int rid_wifi_sniffer_get_last_data(char *out_buf, size_t buf_size)
{
    if (!wifi_sniffer_initialized || rid_mutex == NULL) {
        return -1;  // 未初始化
    }
    
    if (xSemaphoreTake(rid_mutex, portMAX_DELAY) != pdTRUE) {
        return -1;  // 无法获取互斥量
    }
    
    size_t len = strnlen(g_last_rid_data, DATA_BUF_LEN);
    if (len >= buf_size) {
        len = buf_size - 1;  // 预留结尾'\0'
    }
    memcpy(out_buf, g_last_rid_data, len);
    out_buf[len] = '\0';
    
    xSemaphoreGive(rid_mutex);
    return len;
}

// WiFi信道配置（1-14）
static uint8_t channel = 6;
// 最大信道数
static uint8_t max_channel = 14;
// 信道切换间隔（毫秒）
static uint32_t channel_switch_interval = 1000;

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

// 打印数据包信息的任务
static void send_rid_task(void* arg) {
    char *send_buf = NULL;
    while (1) {
        send_buf = (char *)malloc(sizeof(char) * DATA_BUF_LEN);
        if (!send_buf) {
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            continue;
        }
        if (prepare_udp_send(send_buf)) {
            // 有网络才发送
            if (ec200x_is_network_connected() && !is_ota_in_progress()) {
                if (udp_client_send(send_buf, strlen(send_buf),pdMS_TO_TICKS(100)) != 1)
                    ESP_LOGE(TAG, "send fail\n");
            }
        }
        if (send_buf) {
            free(send_buf);
            send_buf = NULL;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1000ms
    }
}

// WiFi数据包捕获回调函数
static void wifi_sniffer_packet_handler(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    
    // 检查数据包长度是否足够包含MAC头部
    if (pkt->rx_ctrl.sig_len < sizeof(wifi_mac_hdr)) {
        return;
    }
    
    wifi_mac_hdr* hdr = (wifi_mac_hdr*)pkt->payload;
    
    // 只处理管理帧
    if (hdr->fc.type != WIFI_FRAME_MGMT) {
        return;
    }

    // 增加管理帧计数器
    // portENTER_CRITICAL(&s_count_mutex);
    // s_mgmt_frame_count++;
    // portEXIT_CRITICAL(&s_count_mutex);

    // 检查是否为需要处理的帧类型
    if (hdr->fc.subtype != WIFI_FRAME_SUBTYPE_BEACON && hdr->fc.subtype != 0x0D) {
        return;
    }

    // ESP_LOG_BUFFER_HEXDUMP(TAG, pkt->payload, pkt->rx_ctrl.sig_len, ESP_LOG_INFO);

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

    // 使用静态缓冲区避免频繁分配内存
    static unsigned char data[MAX_PAYLOAD];
    int beacon_offset = 36;
    int nan_offset = 24;
    int length = 0;

    // 检查Beacon帧
    if (hdr->fc.subtype == WIFI_FRAME_SUBTYPE_BEACON) {
        while(beacon_offset < pkt->rx_ctrl.sig_len) {
            // 处理Beacon帧的内容
            if(pkt->payload[beacon_offset] == 0xDD) { // vendor specific
                if(pkt->payload[beacon_offset+2]==0xFA && 
                   pkt->payload[beacon_offset+3]==0x0B && 
                   pkt->payload[beacon_offset+4]==0xBC && 
                   pkt->payload[beacon_offset+5] == 0x0D) { // Remote ID
                    length = pkt->payload[beacon_offset+1];
                    if (length <= MAX_PAYLOAD) {
                        get_data_from_index(pkt->payload, pkt->rx_ctrl.sig_len, 
                                          beacon_offset, length, data);
                        recv_parse_data(data, packet_info.rssi, channel, 0, 0);
                    }
                }
            }
            beacon_offset++;
        }
    }
    // 检查NAN帧 (子类型13)
    else if (hdr->fc.subtype == 0x0D) {
        while(nan_offset < pkt->rx_ctrl.sig_len) {
            // 处理NAN帧的内容
            if(pkt->payload[nan_offset] == 0x04 && pkt->payload[nan_offset+1] == 0x09) { // Public Action
                if(pkt->payload[nan_offset+2]==0x50 && 
                   pkt->payload[nan_offset+3]==0x6F && 
                   pkt->payload[nan_offset+4]==0x9A && 
                   pkt->payload[nan_offset+5] == 0x13) {
                    length = pkt->payload[nan_offset+18] + 19;
                    if (length <= MAX_PAYLOAD) {
                        get_data_from_index(pkt->payload, pkt->rx_ctrl.sig_len, 
                                          nan_offset, length, data);
                        recv_parse_data(data, packet_info.rssi, channel, 0, 0);
                    }
                }
            }
            nan_offset++;
        }
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

static void mgmt_frame_count_timer_cb(void* arg) {
    uint32_t count;
    // 读取当前计数并清零
    portENTER_CRITICAL(&s_count_mutex);
    count = s_mgmt_frame_count;
    s_mgmt_frame_count = 0;
    portEXIT_CRITICAL(&s_count_mutex);
    
    ESP_LOGI(TAG, "Management frames received in last second: %ld \r\n", count);
}

void rid_wifi_sniffer_init(void) 
{
    // 防止重复初始化
    if (wifi_sniffer_initialized) {
        ESP_LOGW(TAG, "WiFi sniffer already initialized");
        return;
    }
    
    // 创建默认AP网络接口并保存句柄
    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_ap_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi AP netif");
        return;
    }

    // 3. 创建互斥量
    rid_mutex = xSemaphoreCreateMutex();
    if (rid_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

    // 4. 初始化WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5. 设置WiFi模式为AP+STA（共存）
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // 6. 配置AP参数
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP32_RID_Sniffer",     // 自定义热点名称
            .ssid_len = 0,                   // 自动计算长度
            .password = "12345678",          // 密码，至少8位
            .max_connection = 4,             // 最大连接数
            .authmode = WIFI_AUTH_WPA_WPA2_PSK, // 认证方式
            .channel = channel,              // 初始信道（与监听信道一致）
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    // 7. 启动WiFi（同时启动AP和STA）
    ESP_ERROR_CHECK(esp_wifi_start());

    // 8. 设置混杂模式（仅对STA接口有效，但APSTA模式下仍然工作）
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));

    // 9. 设置初始监听信道（此信道同时成为AP的工作信道）
    ESP_ERROR_CHECK(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE));
    
    ESP_LOGI(TAG, "WiFi sniffer started with AP: %s", ap_config.ap.ssid);

    // 10. 可选：不再需要信道切换任务（否则AP信道会频繁变动，导致手机断开）
    // xTaskCreate(channel_switch_task, "channel_switch", 2048, NULL, 5, NULL);

    // 11. 创建数据发送任务
    xTaskCreate(send_rid_task, "send_rid", 8192, NULL, 5, &send_rid_task_handle);

    // // 创建并启动每秒统计定时器
    // esp_timer_create_args_t timer_args = {
    //     .callback = mgmt_frame_count_timer_cb,
    //     .arg = NULL,
    //     .name = "mgmt_count_timer"
    // };
    // ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_count_timer));
    // ESP_ERROR_CHECK(esp_timer_start_periodic(s_count_timer, 1000000)); // 1000000微秒 = 1秒

    // 12. 标记初始化完成
    wifi_sniffer_initialized = true;
}

void free_rid_wifi_sniffer(void)
{   
    if (!wifi_sniffer_initialized) {
        ESP_LOGW(TAG, "WiFi sniffer not initialized, nothing to free");
        return;
    }

    // 1. 停止并删除发送任务
    if (send_rid_task_handle != NULL) {
        vTaskDelete(send_rid_task_handle);
        send_rid_task_handle = NULL;
    }

    // 2. 停止WiFi（同时停止AP和STA）
    ESP_ERROR_CHECK(esp_wifi_stop());

    // 3. 删除互斥量
    if (rid_mutex != NULL) {
        vSemaphoreDelete(rid_mutex);
        rid_mutex = NULL;
    }

    // 4. 清空无线链表
    free_list(&wifi_head);
    wifi_head = NULL;

    // 5. 反初始化WiFi驱动
    ESP_ERROR_CHECK(esp_wifi_deinit());

    // 6. 销毁之前创建的AP网络接口
    if (s_ap_netif != NULL) {
        esp_netif_destroy(s_ap_netif);
        s_ap_netif = NULL;
    }

    // 6. 重置全局状态
    max_rssi = -127;
    loss_rate_value = 0.0f;
    memset(&device_Info, 0, sizeof(device_Info));
    
    wifi_sniffer_initialized = false;
    ESP_LOGI(TAG, "All WiFi sniffer resources released");
}
