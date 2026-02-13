#include "udp_client.h"

// ===================== 配置 =====================
#define TAG "UDP_CLIENT"

#define UDP_SERVER_IP        "112.125.89.8"
#define UDP_SERVER_PORT      33510

#define UDP_HEARTBEAT_PERIOD_MS   (10 * 1000)

#define UDP_SEND_TIMEOUT_MS       2000

// 外部发送队列
#define UDP_TX_QUEUE_LEN          16
#define UDP_TX_MAX_PAYLOAD        2048

static TaskHandle_t s_udp_task = NULL;

// 发送队列：外部任务把数据丢这里
static QueueHandle_t s_tx_queue = NULL;

static bool s_udp_inited = false;

typedef struct {
    uint16_t len;
    uint8_t  data[UDP_TX_MAX_PAYLOAD];
} udp_tx_item_t;


// ===================== 外部发送接口 =====================
// 其他任务调用这个函数，把数据排队发往 UDP 服务器
bool udp_client_send(const void *data, size_t len, TickType_t wait_ticks)
{
   // 没网直接失败，不排队
    if (!ec200x_is_network_connected()) {
        return false;
    }

    // 检查参数
    if (!s_tx_queue || !data || len == 0) {
        ESP_LOGE(TAG, "Invalid parameters: queue=%p, data=%p, len=%u", 
                s_tx_queue, data, (unsigned)len);
        return false;
    }
    
    // 检查数据长度
    if (len > UDP_TX_MAX_PAYLOAD) {
        ESP_LOGE(TAG, "Data too large: %u > %u", (unsigned)len, UDP_TX_MAX_PAYLOAD);
        return false;
    }

    udp_tx_item_t item;
    item.len = (uint16_t)len;
    memcpy(item.data, data, len);

    return xQueueSend(s_tx_queue, &item, wait_ticks) == pdTRUE;
}

// ===================== UDP socket =====================
static int udp_socket_connect(void)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed: errno=%d", errno);
        return -1;
    }

    // 发送超时
    struct timeval tv;
    tv.tv_sec  = UDP_SEND_TIMEOUT_MS / 1000;
    tv.tv_usec = (UDP_SEND_TIMEOUT_MS % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(UDP_SERVER_PORT);

    if (inet_pton(AF_INET, UDP_SERVER_IP, &dest.sin_addr) != 1) {
        ESP_LOGE(TAG, "inet_pton failed for %s", UDP_SERVER_IP);
        close(sock);
        return -1;
    }

    // UDP connect：设置默认对端
    if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
        ESP_LOGE(TAG, "connect() failed: errno=%d", errno);
        close(sock);
        return -1;
    }

    ESP_LOGI(TAG, "UDP connected to %s:%d", UDP_SERVER_IP, UDP_SERVER_PORT);
    return sock;
}

static bool udp_send_one(int sock, const void *data, size_t len)
{
    int ret = send(sock, data, len, 0);
    if (ret < 0) {
        ESP_LOGW(TAG, "send() failed: errno=%d", errno);
        return false;
    }
    return true;
}

// ===================== 心跳/定时数据 =====================
static void build_heartbeat(uint8_t *out, size_t *out_len)
{
    const char *hb = "HB\n";
    size_t n = strlen(hb);
    memcpy(out, hb, n);
    *out_len = n;
}

// ===================== UDP 任务 =====================
static void udp_task(void *arg)
{
    (void)arg;

    while (1) {
        // ---------- 1. 阻塞等待网络（永不超时）----------
        ESP_LOGI(TAG, "Waiting for network...");
        // ec200x_wait_for_network(portMAX_DELAY);
        while(!ec200x_is_network_connected())
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        ESP_LOGI(TAG, "Network is up, creating UDP socket...");

        // ---------- 2. 创建并连接 UDP socket ----------
        int sock = udp_socket_connect();
        if (sock < 0) {
            ESP_LOGE(TAG, "Failed to create socket, retry in 5s");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;   // 重新等待网络（网络实际已通，但 socket 建失败）
        }

        TickType_t last_hb = xTaskGetTickCount();
        TickType_t last_dat = xTaskGetTickCount();

        // ---------- 3. 网络监控循环（每 5 秒检查一次）----------
        while (1) {
            // 优先处理外部排队数据
            udp_tx_item_t item;
            if (s_tx_queue && xQueueReceive(s_tx_queue, &item, 0) == pdTRUE) {
                // ESP_LOGI(TAG, "send queued (%u bytes)", (unsigned)item.len);
                if (!udp_send_one(sock, item.data, item.len)) {
                    ESP_LOGW(TAG, "send failed, close socket");
                    close(sock);
                    sock = -1;
                    break;      // 退出内循环，重新等待网络
                }
            }

            TickType_t now = xTaskGetTickCount();

            // 心跳发送
            if ((now - last_hb) * portTICK_PERIOD_MS >= UDP_HEARTBEAT_PERIOD_MS) {
                uint8_t hb[32];
                size_t hb_len = 0;
                build_heartbeat(hb, &hb_len);
                ESP_LOGI(TAG, "send heartbeat (%u bytes)", (unsigned)hb_len);
                if (!udp_send_one(sock, hb, hb_len)) {
                    ESP_LOGW(TAG, "heartbeat send failed, close socket");
                    close(sock);
                    sock = -1;
                    break;
                }
                last_hb = now;
            }

            // 定期检查网络状态（5秒一次）
            static TickType_t last_net_check = 0;
            if ((now - last_net_check) * portTICK_PERIOD_MS >= 5000) {
                last_net_check = now;
                if (!ec200x_is_network_connected()) {
                    ESP_LOGW(TAG, "Network disconnected, cleanup and wait...");
                    close(sock);
                    sock = -1;
                    break;      // 退出内循环，重新等待网络
                }
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }

        // socket 已关闭，稍等片刻再进入下一轮等待网络
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ===================== 对外初始化 =====================
void udp_client_init(void)
{
    if (s_udp_inited) {
        ESP_LOGW(TAG, "UDP client already initialized");
        return;
    }

    if (!s_tx_queue) {
        s_tx_queue = xQueueCreate(UDP_TX_QUEUE_LEN, sizeof(udp_tx_item_t));
    }

    if (!s_udp_task) {
        xTaskCreate(udp_task, "udp_task", 1024 * 30, NULL, 8, &s_udp_task);
    }

    s_udp_inited = true;
    ESP_LOGI(TAG, "udp_client_init done");
}

void udp_client_free(void)
{
    if (!s_udp_inited) {
        ESP_LOGW(TAG, "UDP client not initialized, nothing to free");
        return;
    }

    ESP_LOGI(TAG, "Freeing UDP client resources...");

    // 1. 删除 UDP 任务（直接强制删除）
    if (s_udp_task) {
        vTaskDelete(s_udp_task);
        s_udp_task = NULL;
    }

    // 2. 删除发送队列
    if (s_tx_queue) {
        vQueueDelete(s_tx_queue);
        s_tx_queue = NULL;
    }

    // 3. 重置初始化标志
    s_udp_inited = false;

    ESP_LOGI(TAG, "UDP client freed");
}