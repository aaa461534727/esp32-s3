#include "ec200x.h"

// ===================== 用户配置 =====================
#define TAG "EC200X_PPP"
static bool s_uart_inited = false;

// UART
#define UART_PORT_NUM      UART_NUM_1
#define UART_TX_PIN        4
#define UART_RX_PIN        5
#define UART_BAUDRATE      115200

// 强烈建议硬件流控（PPP 长时间稳定性更好）
#define UART_RTS_PIN       UART_PIN_NO_CHANGE
#define UART_CTS_PIN       UART_PIN_NO_CHANGE

// PPP 拨号 / APN
#define APN_STRING         "CMNET"
#define DIAL_CMD           "ATD*99#"

// 应用层探测（可选，不替代 LCP Echo）
#define KEEPALIVE_PERIOD_MS        (30 * 1000)
#define KEEPALIVE_FAIL_THRESHOLD   2
#define PROBE_HOST   "www.baidu.com"
#define PROBE_PORT   "80"
#define PROBE_TIMEOUT_MS  5000

// UART RX
#define UART_RX_BUF_SIZE   (4 * 1024)
#define UART_TX_BUF_SIZE   (2 * 1024)

// ===================== 全局状态 =====================
static esp_netif_t *s_ppp_netif = NULL;
static EventGroupHandle_t s_evt = NULL;

// 保存 PPP 网络信息的结构体
static struct {
    char ip[16];   // 保存 PPP 获取到的 IP 地址
    char mask[16]; // 保存子网掩码
    char gw[16];   // 保存网关地址
    bool valid;    // 标记信息是否有效
} s_ppp_network_info = {0};

#define EVT_PPP_GOT_IP     BIT0
#define EVT_PPP_LOST_IP    BIT1
#define EVT_PPP_ERR_DEAD   BIT2   // 对端超时/死链路（Connection timeout）

// RX task 只创建一次
static TaskHandle_t s_rx_task = NULL;

static bool s_network_connected = false;   // PPP已获取IP则为true
// 查询当前网络是否已连接（非阻塞）
bool ec200x_is_network_connected(void)
{
    return s_network_connected;
}

// 等待网络连接，可指定超时（单位：系统Tick）
// 成功连接返回 pdTRUE，超时返回 pdFALSE
BaseType_t ec200x_wait_for_network(TickType_t timeout_ticks)
{
    EventBits_t bits = xEventGroupWaitBits(s_evt, EVT_PPP_GOT_IP,
                                            pdFALSE, pdFALSE, timeout_ticks);
    return (bits & EVT_PPP_GOT_IP) ? pdTRUE : pdFALSE;
}

// 获取 PPP 的 IP、子网掩码和网关
void get_ppp_ipv4_gw_mask(char *ip, size_t ip_len, char *mask, size_t mask_len, char *gw, size_t gw_len)
{
    if (ip && ip_len) {
        if (s_ppp_network_info.valid) {
            snprintf(ip, ip_len, "%s", s_ppp_network_info.ip);
        } else {
            ESP_LOGW(TAG, "PPP IP not valid");
            snprintf(ip, ip_len, "0.0.0.0");
        }
    }
    
    if (mask && mask_len) {
        if (s_ppp_network_info.valid) {
            snprintf(mask, mask_len, "%s", s_ppp_network_info.mask);
        } else {
            snprintf(mask, mask_len, "0.0.0.0");
        }
    }
    
    if (gw && gw_len) {
        if (s_ppp_network_info.valid) {
            snprintf(gw, gw_len, "%s", s_ppp_network_info.gw);
        } else {
            snprintf(gw, gw_len, "0.0.0.0");
        }
    }
}

// 自定义 I/O 驱动对象：必须把 esp_netif_driver_base_t 放第一个成员
typedef struct {
    esp_netif_driver_base_t base;   // must be first
    esp_netif_t *netif;
} ppp_uart_driver_t;

static ppp_uart_driver_t *s_drv = NULL;

// ===================== UART 基础 =====================
static esp_err_t uart_init_ppp(void)
{
    if (s_uart_inited) {
        uart_flush(UART_PORT_NUM);
        return ESP_OK;
    }

    uart_config_t cfg = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = (UART_RTS_PIN == UART_PIN_NO_CHANGE || UART_CTS_PIN == UART_PIN_NO_CHANGE)
                        ? UART_HW_FLOWCTRL_DISABLE
                        : UART_HW_FLOWCTRL_CTS_RTS,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_RTS_PIN, UART_CTS_PIN));
    uart_flush(UART_PORT_NUM);

    s_uart_inited = true;
    return ESP_OK;
}


static void uart_send_str(const char *s)
{
    if (!s) return;
    uart_write_bytes(UART_PORT_NUM, s, (int)strlen(s));
}

static void uart_send_line(const char *cmd)
{
    if (!cmd) return;
    uart_send_str(cmd);
    uart_send_str("\r");
}

static bool send_at_cmd(const char *cmd, int timeout_ms)
{
    uart_flush_input(UART_PORT_NUM);

    if (cmd && cmd[0]) {
        ESP_LOGI(TAG, "AT> %s", cmd);
        uart_send_line(cmd);
    }

    const int64_t t_end = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    char *buf = (char *)calloc(1, 512);
    if (!buf) return false;

    size_t used = 0;
    while (esp_timer_get_time() < t_end) {
        uint8_t ch;
        int r = uart_read_bytes(UART_PORT_NUM, &ch, 1, pdMS_TO_TICKS(20));
        if (r == 1) {
            if (used < 510) buf[used++] = (char)ch;

            if (strstr(buf, "\r\nOK\r\n") || strstr(buf, "\nOK\r\n") || strstr(buf, "\r\nOK\n")) {
                ESP_LOGI(TAG, "AT< OK");
                free(buf);  //
                return true;
            }
            if (strstr(buf, "\r\nERROR\r\n") || strstr(buf, "\nERROR\r\n") ||
                strstr(buf, "+CME ERROR") || strstr(buf, "+CMS ERROR")) {
                ESP_LOGW(TAG, "AT< ERROR: %s", buf);
                free(buf);  // 
                return false;
            }
        }
    }

    ESP_LOGW(TAG, "AT timeout: %s, resp(part)=%s", cmd ? cmd : "(null)", buf);
    free(buf);  // 
    return false;
}

// ===================== ESP-NETIF 驱动对接（PPP over UART） =====================

// transmit：lwIP/PPP 栈要发数据时会调用这个，把 PPP 帧写到 UART
static esp_err_t ppp_uart_transmit(void *h, void *buffer, size_t len)
{
    (void)h;
    if (!buffer || len == 0) return ESP_OK;
    int w = uart_write_bytes(UART_PORT_NUM, (const char *)buffer, (int)len);
    return (w >= 0) ? ESP_OK : ESP_FAIL;
}

// free_rx_buffer：PPP 栈处理完 RX buffer 后回调释放
static void ppp_uart_free_rx(void *h, void *buffer)
{
    (void)h;
    free(buffer);
}

// post_attach：在 esp_netif_attach() 时被调用
static esp_err_t ppp_uart_post_attach(esp_netif_t *esp_netif, void *args)
{
    ppp_uart_driver_t *drv = (ppp_uart_driver_t *)args;
    drv->netif = esp_netif;

    const esp_netif_driver_ifconfig_t driver_ifconfig = {
        .handle = drv,
        .transmit = ppp_uart_transmit,
        .driver_free_rx_buffer = NULL,
    };

    ESP_ERROR_CHECK(esp_netif_set_driver_config(esp_netif, &driver_ifconfig));
    return ESP_OK;
}

static void uart_ppp_rx_task(void *arg)
{
    (void)arg;
    static uint8_t buf[1024];   // 静态缓冲，避免碎片化

    while (1) {
        esp_netif_t *netif = s_ppp_netif;
        if (!netif) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        int r = uart_read_bytes(UART_PORT_NUM, buf, sizeof(buf), pdMS_TO_TICKS(1000));
        if (r > 0) {
            // esp_netif_receive 内部会拷贝到 pbuf（走标准路径）
            esp_err_t err = esp_netif_receive(netif, buf, (size_t)r, NULL);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "esp_netif_receive err=%s", esp_err_to_name(err));
            }
        }
    }
}


// ===================== PPP / IP 事件 =====================
static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base;

    if (id == IP_EVENT_PPP_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "PPP GOT IP: " IPSTR, IP2STR(&e->ip_info.ip));
        ESP_LOGI(TAG, "NETMASK: " IPSTR ", GW: " IPSTR,
                 IP2STR(&e->ip_info.netmask), IP2STR(&e->ip_info.gw));
        // 保存 IP、子网掩码和网关
        snprintf(s_ppp_network_info.ip, sizeof(s_ppp_network_info.ip), IPSTR, IP2STR(&e->ip_info.ip));
        snprintf(s_ppp_network_info.mask, sizeof(s_ppp_network_info.mask), IPSTR, IP2STR(&e->ip_info.netmask));
        snprintf(s_ppp_network_info.gw, sizeof(s_ppp_network_info.gw), IPSTR, IP2STR(&e->ip_info.gw));
        s_ppp_network_info.valid = true;  // 设置有效标记
        xEventGroupClearBits(s_evt, EVT_PPP_LOST_IP | EVT_PPP_ERR_DEAD);
        xEventGroupSetBits(s_evt, EVT_PPP_GOT_IP);
        s_network_connected = true;
        rid_wifi_sniffer_init();
        iot_init();
        udp_client_init();
    } else if (id == IP_EVENT_PPP_LOST_IP) {
        ESP_LOGW(TAG, "PPP LOST IP");
        xEventGroupClearBits(s_evt, EVT_PPP_GOT_IP);
        xEventGroupSetBits(s_evt, EVT_PPP_LOST_IP);
        s_network_connected = false;
        s_ppp_network_info.valid = false;  // 断开时标记无效
    }
}

static void on_ppp_status_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)data;

    static const char* phase_names[] = {
        "DEAD", "MASTER", "HOLDOFF", "INITIALIZE", "SERIALCONN",
        "DORMANT", "ESTABLISH", "AUTHENTICATE", "CALLBACK",
        "NETWORK", "RUNNING", "TERMINATE", "DISCONNECT"
    };

    static const char* error_names[] = {
        "ERRORNONE", "ERRORPARAM", "ERROROPEN", "ERRORDEVICE",
        "ERRORALLOC", "ERRORUSER", "ERRORCONNECT", "ERRORAUTHFAIL",
        "ERRORPROTOCOL", "ERRORPEERDEAD", "ERRORIDLETIMEOUT",
        "ERRORCONNECTTIME", "ERRORLOOPBACK"
    };

    // 对端死亡事件
    if (id == NETIF_PPP_ERRORPEERDEAD) {
        ESP_LOGE(TAG, "PPP status: PEERDEAD (Connection timeout) -> redial");
        xEventGroupSetBits(s_evt, EVT_PPP_ERR_DEAD);
    } 
    // 空闲超时事件（重要！）
    else if (id == NETIF_PPP_ERRORIDLETIMEOUT) {
        ESP_LOGE(TAG, "PPP status: IDLE TIMEOUT -> redial");
        xEventGroupSetBits(s_evt, EVT_PPP_ERR_DEAD);
    }
    // 连接超时事件
    else if (id == NETIF_PPP_ERRORCONNECTTIME) {
        ESP_LOGE(TAG, "PPP status: CONNECT TIME LIMIT REACHED -> redial");
        xEventGroupSetBits(s_evt, EVT_PPP_ERR_DEAD);
    }
    // 其他错误事件
    else if (id >= NETIF_PPP_ERRORNONE && id <= NETIF_PPP_ERRORLOOPBACK) {
        ESP_LOGW(TAG, "PPP error event: %s", error_names[id]);
    }
    // 阶段事件
    else if (id >= NETIF_PP_PHASE_OFFSET && id <= NETIF_PPP_PHASE_DISCONNECT) {
        int phase_idx = id - NETIF_PP_PHASE_OFFSET;
        if (phase_idx >= 0 && phase_idx < (int)(sizeof(phase_names)/sizeof(phase_names[0]))) {
            ESP_LOGI(TAG, "PPP phase: %s", phase_names[phase_idx]);
        }
        
        // 重要阶段记录
        if (id == NETIF_PPP_PHASE_ESTABLISH) {
            ESP_LOGI(TAG, "PPP: Link establishment starting");
        } else if (id == NETIF_PPP_PHASE_AUTHENTICATE) {
            ESP_LOGI(TAG, "PPP: Authentication phase");
        } else if (id == NETIF_PPP_PHASE_NETWORK) {
            ESP_LOGI(TAG, "PPP: Network layer protocol phase");
        } else if (id == NETIF_PPP_PHASE_RUNNING) {
            ESP_LOGI(TAG, "PPP: Link is up and running");
        } else if (id == NETIF_PPP_PHASE_DEAD) {
            ESP_LOGW(TAG, "PPP: Link is dead");
        }
    }
}

// ===================== 拨号 / 重拨 =====================
static void ppp_stop_and_cleanup(void)
{   
    free_rid_wifi_sniffer();
    free_iot_init();
    udp_client_free();
    if (s_ppp_netif) {
        // 1. 发起停止命令
        esp_netif_action_stop(s_ppp_netif, NULL, 0, NULL);
        // 2. 必须等待！让 LWIP 有时间处理终止包、移除定时器
        vTaskDelay(pdMS_TO_TICKS(200));
        // 3. 销毁 netif
        esp_netif_destroy(s_ppp_netif);
        s_ppp_netif = NULL;
        
    }

    if (s_drv) {
        free(s_drv);
        s_drv = NULL;
    }

    if (s_uart_inited) {
        uart_flush(UART_PORT_NUM);
    }

    // 可选：清除事件组，避免残留 IP 标志
    xEventGroupClearBits(s_evt, EVT_PPP_GOT_IP | EVT_PPP_ERR_DEAD | EVT_PPP_LOST_IP);
}

static bool ppp_start_and_dial(void)
{
    ESP_ERROR_CHECK(uart_init_ppp());

    // 尝试退出透明态（有些模组需要）
    uart_send_str("+++");
    vTaskDelay(pdMS_TO_TICKS(1200));
    uart_flush(UART_PORT_NUM);

    if (!send_at_cmd("AT", 800)) return false;
    send_at_cmd("ATE0", 500);
    send_at_cmd("AT+CPIN?", 800);
    send_at_cmd("AT+CREG?", 800);
    send_at_cmd("AT+CGREG?", 800);
    send_at_cmd("AT+CSQ", 800);

    char cgdcont[128];
    snprintf(cgdcont, sizeof(cgdcont), "AT+CGDCONT=1,\"IP\",\"%s\"", APN_STRING);
    send_at_cmd(cgdcont, 1000);

    // 创建 PPP netif
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_PPP();
    s_ppp_netif = esp_netif_new(&cfg);
    if (!s_ppp_netif) {
        ESP_LOGE(TAG, "esp_netif_new(PPP) failed");
        return false;
    }

    // PPP 参数：打开 phase/error 事件；若编译期开了 LCP ECHO，这里确保不禁用
    esp_netif_ppp_config_t ppp_cfg = {
        .ppp_phase_event_enabled = true,
        .ppp_error_event_enabled = true,
#ifdef CONFIG_LWIP_ENABLE_LCP_ECHO
        .ppp_lcp_echo_disabled = false,
#endif
    };
    // 打印当前的 LCP 设置
    esp_netif_ppp_config_t ppp_cfg_read;
    ESP_ERROR_CHECK(esp_netif_ppp_set_params(s_ppp_netif, &ppp_cfg));
    ESP_ERROR_CHECK(esp_netif_ppp_get_params(s_ppp_netif, &ppp_cfg_read));
    
    ESP_LOGI(TAG, "PPP Config - Phase Events: %s, Error Events: %s", 
             ppp_cfg_read.ppp_phase_event_enabled ? "Enabled" : "Disabled",
             ppp_cfg_read.ppp_error_event_enabled ? "Enabled" : "Disabled");
#ifdef CONFIG_LWIP_ENABLE_LCP_ECHO
    ESP_LOGI(TAG, "PPP Config - LCP Echo Disabled: %s", 
             ppp_cfg_read.ppp_lcp_echo_disabled ? "Yes" : "No");
#endif

    // attach 自定义 UART 驱动
    s_drv = (ppp_uart_driver_t *)calloc(1, sizeof(ppp_uart_driver_t));
    if (!s_drv) return false;
    s_drv->base.post_attach = ppp_uart_post_attach;
    ESP_ERROR_CHECK(esp_netif_attach(s_ppp_netif, s_drv));

    esp_netif_set_default_netif(s_ppp_netif);
    esp_netif_action_start(s_ppp_netif, NULL, 0, NULL);

    // RX task 只创建一次
    if (!s_rx_task) {
        xTaskCreate(uart_ppp_rx_task, "uart_ppp_rx", 4096, NULL, 12, &s_rx_task);
    }

    ESP_LOGI(TAG, "Dial: %s", DIAL_CMD);
    uart_send_line(DIAL_CMD);

    vTaskDelay(pdMS_TO_TICKS(1200));

    EventBits_t bits = xEventGroupWaitBits(
        s_evt, EVT_PPP_GOT_IP, pdFALSE, pdFALSE, pdMS_TO_TICKS(60000)
    );

    if (bits & EVT_PPP_GOT_IP) {
        ESP_LOGI(TAG, "PPP is up");
        return true;
    }

    ESP_LOGW(TAG, "PPP up timeout, will redial");
    return false;
}

// ===================== 改进的保活机制 =====================

// 添加一个简单的 HTTP GET 请求作为心跳
static bool http_keepalive_once(void)
{
    ESP_LOGI(TAG, "Sending HTTP keepalive...");
    
    struct addrinfo hints = {0};
    struct addrinfo *res = NULL;
    char request[256];
    int sockfd, ret;
    
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    ret = getaddrinfo("www.baidu.com", "80", &hints, &res);
    if (ret != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS resolution failed!");
        return false;
    }
    
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        ESP_LOGE(TAG, "Socket creation failed: %d", errno);
        freeaddrinfo(res);
        return false;
    }
    
    // 设置超时
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    // 连接
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        ESP_LOGE(TAG, "Connect failed: %d", errno);
        close(sockfd);
        freeaddrinfo(res);
        return false;
    }
    
    freeaddrinfo(res);
    
    // 发送 HTTP GET 请求
    snprintf(request, sizeof(request),
             "GET / HTTP/1.1\r\n"
             "Host: www.baidu.com\r\n"
             "User-Agent: ESP32-PPP/1.0\r\n"
             "Connection: close\r\n"
             "\r\n");
    
    ret = send(sockfd, request, strlen(request), 0);
    if (ret < 0) {
        ESP_LOGE(TAG, "Send failed: %d", errno);
        close(sockfd);
        return false;
    }
    
    // 读取一些响应（不需要完整读取）
    char buf[128];
    ret = recv(sockfd, buf, sizeof(buf)-1, 0);
    if (ret > 0) {
        buf[ret] = '\0';
        ESP_LOGI(TAG, "HTTP response received (%d bytes)", ret);
    }
    
    close(sockfd);
    
    // 只要发送成功就算保活成功
    ESP_LOGI(TAG, "HTTP keepalive successful");
    return true;
}

// ===================== 修改后的 ppp_keepalive_task =====================
static void ppp_keepalive_task(void *arg)
{
    (void)arg;
    bool connected = false;

    while (1) {
        // ----- 1. 未连接：持续重拨（固定5秒间隔）-----
        while (!connected) {
            ESP_LOGI(TAG, "Attempting PPP dial-up...");
            if (ppp_start_and_dial()) {
                connected = true;
                ESP_LOGI(TAG, "PPP connection established");
                break;
            }
            ESP_LOGW(TAG, "PPP dial failed, retry after 5s");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        // ----- 2. 已连接：保活 + 监控断线 -----
        int fail_cnt = 0;
        TickType_t last_success = xTaskGetTickCount();

        while (connected) {
            EventBits_t bits = xEventGroupGetBits(s_evt);

            // --- 2.1 PPP底层报告断线 ---
            if (bits & (EVT_PPP_ERR_DEAD | EVT_PPP_LOST_IP)) {
                ESP_LOGW(TAG, "PPP link dead or IP lost");
                xEventGroupClearBits(s_evt, EVT_PPP_ERR_DEAD | EVT_PPP_LOST_IP | EVT_PPP_GOT_IP);
                ppp_stop_and_cleanup();   // ← 立即清理
                connected = false;
                s_network_connected = false;
                break;
            }

            // --- 2.2 应用层保活 ---
            TickType_t now = xTaskGetTickCount();
            TickType_t elapsed = (now - last_success) * portTICK_PERIOD_MS;
            if (elapsed >= KEEPALIVE_PERIOD_MS) {
                if (http_keepalive_once()) {
                    fail_cnt = 0;
                    last_success = now;
                } else {
                    fail_cnt++;
                    ESP_LOGW(TAG, "Keepalive FAIL (%d/%d)", fail_cnt, KEEPALIVE_FAIL_THRESHOLD);
                }

                if (fail_cnt >= KEEPALIVE_FAIL_THRESHOLD) {
                    ESP_LOGW(TAG, "Keepalive threshold reached, reconnect");
                    ppp_stop_and_cleanup();   // ← 立即清理
                    connected = false;
                    s_network_connected = false;
                    break;
                }
            }

            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // 断线后稍等，避免疯狂重试
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ===================== 对外初始化入口（非阻塞版） =====================
void ec200x_init(void)
{
    static bool inited = false;
    if (inited) return;
    inited = true;

    s_evt = xEventGroupCreate();
    xEventGroupClearBits(s_evt, EVT_PPP_GOT_IP | EVT_PPP_LOST_IP | EVT_PPP_ERR_DEAD);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_status_event, NULL));

    // 创建后台任务，立即返回，不阻塞
    xTaskCreate(ppp_keepalive_task, "ppp_keepalive", 4096, NULL, 8, NULL);

    ESP_LOGI(TAG, "ec200x_init done (non-blocking)");
}