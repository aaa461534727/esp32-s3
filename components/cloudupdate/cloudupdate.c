#include "cloudupdate.h"
/* ------------------------- 配置宏定义 ------------------------- */
#define CLOUD_UPG_VERSION       "3.0"
#define LOCK_SECONDS            3600                // 检查间隔
#define SOCK_RW_TIMEOUT         10                  // 网络超时(秒)
#define OTA_BUF_SIZE            12*1024                // OTA写入缓冲区大小
#define MAX_HTTP_OUTPUT_BUFFER  2048

// NVS命名空间
#define NVS_NAMESPACE           "cloud_upd"

// NVS键名
#define NVS_KEY_HOST          "host"
#define NVS_KEY_PATH          "path"
#define NVS_KEY_PORT          "port"
#define NVS_KEY_TIMESTAMP     "ts"
#define NVS_KEY_NETCHECK      "netchk"
#define NVS_KEY_MODE          "mode"
#define NVS_KEY_URL           "url"
#define NVS_KEY_MAGICID       "magic"
#define NVS_KEY_VERSION       "version"
#define NVS_KEY_APRULE        "aprule"
#define NVS_KEY_UPTIME        "uptime"
#define NVS_KEY_STATUS        "status"
#define NVS_KEY_VENDORCODE    "vendor"

// 默认服务器配置
#define DEFAULT_HOST          "update.cpe-iot.com"
#define DEFAULT_PATH          "/jeecgboot/ota/api/upgrade/check"
#define DEFAULT_PORT          80

// 日志标签
static const char *TAG = "CLOUD_UPDATE";

/* ------------------------- 枚举与结构体 ------------------------- */
typedef enum {
    UPG_IDLE = 0,
    UPG_MANUAL,
    UPG_AUTO,       // 强制升级
    UPG_MAX
} upg_mode_t;

typedef struct {
    uint32_t total;
    uint32_t free;
} meminfo_t;

/* ------------------------- 静态全局变量 ------------------------- */
static char g_cloud_host[64] = DEFAULT_HOST;
static char g_cloud_path[128] = DEFAULT_PATH;
static int  g_cloud_port = DEFAULT_PORT;

/* ------------------------- OTA函数前置声明 ------------------------- */
static void perform_ota(const char *url);

/* ------------------------- NVS 读写封装 ------------------------- */
static esp_err_t nvs_get_str_def(const char *key, char *out, size_t len, const char *def_val)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        strlcpy(out, def_val, len);
        return err;
    }
    size_t required = len;
    err = nvs_get_str(nvs, key, out, &required);
    if (err != ESP_OK) {
        strlcpy(out, def_val, len);
    }
    nvs_close(nvs);
    return err;
}

static esp_err_t nvs_set_str_val(const char *key, const char *val)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_str(nvs, key, val);
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    nvs_close(nvs);
    return err;
}

static esp_err_t nvs_get_int_val(const char *key, int *out, int def_val)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        *out = def_val;
        return err;
    }
    int32_t val = def_val;
    err = nvs_get_i32(nvs, key, &val);
    if (err != ESP_OK) {
        *out = def_val;
    } else {
        *out = (int)val;
    }
    nvs_close(nvs);
    return err;
}

static esp_err_t nvs_set_int_val(const char *key, int val)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_i32(nvs, key, (int32_t)val);
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    nvs_close(nvs);
    return err;
}

/* ------------------------- 设备信息获取 ------------------------- */
static void get_device_mac(char *mac_str)
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(mac_str, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void get_device_csid(char *csid, size_t len)
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(csid, len, "ESP32_%02x%02x%02x", mac[3], mac[4], mac[5]);
}

static void get_device_version(char *version, size_t len)
{
    const esp_app_desc_t *desc = esp_app_get_description();
    snprintf(version, len, "%s", desc->version);
}

static void get_device_svn(char *svn, size_t len)
{
    // 模拟SVN号，可用编译时间戳等
    snprintf(svn, len, "%lld", esp_timer_get_time() / 1000000);
}

static void get_device_model(char *model, size_t len)
{
    snprintf(model, len, "TEST_DEV");
}

static void get_device_runner(char *runner, size_t len)
{
    strlcpy(runner, "default", len);
}

static void get_device_cid(char *cid, size_t len)
{
    strlcpy(cid, "0", len);
}

static void get_device_vendorcode(char *vc, size_t len)
{
    nvs_get_str_def(NVS_KEY_VENDORCODE, vc, len, "0");
}

/* ------------------------- 内存与Flash信息 ------------------------- */
static uint32_t get_flash_total_size(void)
{
    uint32_t size;
    esp_flash_get_size(NULL, &size);
    return size;
}

static int check_machine_memory(meminfo_t *m)
{
    m->total = esp_get_free_heap_size();
    m->free  = esp_get_free_heap_size();
    return 1;
}

/* ------------------------- 时间戳检查 ------------------------- */
static int need_update_stamp(uint32_t *now_sec)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *now_sec = tv.tv_sec;

    int last_ts = 0;
    nvs_get_int_val(NVS_KEY_TIMESTAMP, &last_ts, 0);
    // 修复：使用 %d 打印 int 类型
    ESP_LOGI(TAG, "Current timestamp %d, last %d, diff %d",
             (int)*now_sec, last_ts, (int)(*now_sec - last_ts));
    return ((*now_sec - last_ts) > LOCK_SECONDS) ? 1 : 0;
}

static void exe_update_stamp(uint32_t tv_sec)
{
    nvs_set_int_val(NVS_KEY_TIMESTAMP, (int)tv_sec);
    nvs_set_int_val(NVS_KEY_NETCHECK, 1);
}

/* ------------------------- HTTP POST 请求 ------------------------- */
static esp_err_t http_post_ready(char *http_h, char *http_b, size_t b_size)
{
    char mac[18] = {0}, csid[32] = {0}, version[32] = {0}, svn[32] = {0};
    char model[32] = {0}, runner[32] = {0}, cid[32] = {0}, vendorcode[32] = {0};

    get_device_mac(mac);
    get_device_csid(csid, sizeof(csid));
    get_device_version(version, sizeof(version));
    get_device_svn(svn, sizeof(svn));
    get_device_model(model, sizeof(model));
    get_device_runner(runner, sizeof(runner));
    get_device_cid(cid, sizeof(cid));
    get_device_vendorcode(vendorcode, sizeof(vendorcode));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "protocol", CLOUD_UPG_VERSION);
    cJSON_AddStringToObject(root, "mac", "E6:BF:36:37:66:00");
    cJSON_AddStringToObject(root, "csid", "C8888");
    cJSON_AddStringToObject(root, "model", model);
    cJSON_AddStringToObject(root, "version", version);
    cJSON_AddStringToObject(root, "svn", "9");
    cJSON_AddStringToObject(root, "vendorcode", "");
    cJSON_AddStringToObject(root, "runner", runner);
    cJSON_AddStringToObject(root, "cid", cid);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    snprintf(http_b, b_size, "%s", json_str);
    free(json_str);
    cJSON_Delete(root);

    snprintf(http_h, 512,
        "POST %s HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "Content-Length: %zu\r\n"
        "Content-Type: application/json\r\n"
        "\r\n",
        g_cloud_path, g_cloud_host, g_cloud_port, strlen(http_b));

    ESP_LOGI(TAG, "HTTP Request JSON: %s", http_b);
    return ESP_OK;
}

/* ------------------------- OTA 升级执行 ------------------------- */
// 计算超时时间
static int64_t get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL + tv.tv_usec / 1000);
}

// HTTP OTA任务
static void http_ota_task(const char *url)
{
    esp_err_t err;
    const esp_partition_t *update_partition = NULL;
    esp_ota_handle_t update_handle = 0;
    int total_bytes_received = 0;
    int data_read;
    char ota_buffer[2048];  // 增大缓冲区

    ESP_LOGI(TAG, "WAITING FOR NETWORK CONNECTION...");

    // 等待网络连接
    int retry_count = 0;
    while (!ec200x_is_network_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
        if (retry_count > 30) {
            ESP_LOGE(TAG, "网络连接超时");
            vTaskDelete(NULL);
            return;
        }
    }
    
    ESP_LOGI(TAG, "START HTTP OTA update，URL: %s", url);
    
    // 配置HTTP客户端 - 增加超时时间
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 120000,           // 增加到120秒
        .buffer_size = 4096,            // 增大缓冲区
        .keep_alive_enable = true,
        .max_redirection_count = 3,
        .disable_auto_redirect = false,
        .user_agent = "ESP32 OTA Client", // 添加User-Agent
    };
    
    // 初始化HTTP客户端
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP client initialization failed");
        vTaskDelete(NULL);
        return;
    }
    
    // 设置接收超时
    esp_http_client_set_timeout_ms(client, 30000); // 30秒接收超时
    
    // 打开连接
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }
    
    // 获取内容长度
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) {
        ESP_LOGE(TAG, "Invalid content length or failed to fetch headers");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Firmware Length: %d bytes", content_length);

    // 获取OTA更新分区
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "Failed to find OTA update partition");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }
    
    // 验证分区大小是否足够
    if (content_length > update_partition->size) {
        ESP_LOGE(TAG, "Firmware is too large for the available OTA partition!");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }
    
    // 开始OTA更新
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to begin OTA update: %s", esp_err_to_name(err));
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Starting to download firmware...");
    
    int64_t start_time = get_time_ms();
    int64_t last_progress_time = start_time;
    int last_progress_bytes = 0;
    
    // 循环读取数据并写入OTA分区
    while (total_bytes_received < content_length) {
        // 检查连接状态
        if (!ec200x_is_network_connected()) {
            ESP_LOGE(TAG, "Network connection lost");
            break;
        }
        
        // 从HTTP流读取数据
        data_read = esp_http_client_read(client, ota_buffer, sizeof(ota_buffer));
        
        if (data_read > 0) {
            // 写入OTA分区
            err = esp_ota_write(update_handle, ota_buffer, data_read);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to write OTA partition: %s", esp_err_to_name(err));
                esp_ota_abort(update_handle);
                break;
            }
            
            total_bytes_received += data_read;
            
            // 每收到32KB打印一次进度，或每5秒打印一次
            int64_t current_time = get_time_ms();
            if (total_bytes_received % 32768 == 0 || 
                current_time - last_progress_time >= 5000) {
                int percent = (total_bytes_received * 100) / content_length;
                int speed = 0;
                if (current_time > start_time) {
                    speed = (total_bytes_received * 1000) / (current_time - start_time);
                }

                ESP_LOGI(TAG, "Download progress: %d/%d bytes (%d%%) Speed: %d bytes/sec", 
                         total_bytes_received, content_length, percent, speed);
                
                last_progress_time = current_time;
                last_progress_bytes = total_bytes_received;
            }
            
            // 检查是否卡住（10秒内没有进展）
            if (current_time - last_progress_time >= 30000 && 
                total_bytes_received == last_progress_bytes) {
                ESP_LOGW(TAG, "Download seems stuck, trying to continue...");
                // 尝试重置接收缓冲区
                esp_http_client_fetch_headers(client);
            }
        } 
        else if (data_read == 0) {
            // 没有数据，等待后重试
            ESP_LOGW(TAG, "No data received, waiting 1 second...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            
            // 检查超时
            if (get_time_ms() - start_time > 300000) { // 5分钟超时
                ESP_LOGE(TAG, "Download timeout after 5 minutes");
                break;
            }
            
            // 尝试重新获取数据
            continue;
        } 
        else {
            // 读取错误
            ESP_LOGE(TAG, "Failed to read data: %d, Error: %s", 
                     data_read, esp_err_to_name(esp_http_client_get_errno(client)));
            
            // 尝试恢复连接
            int retry = 0;
            while (retry < 3 && total_bytes_received < content_length) {
                ESP_LOGI(TAG, "Attempting to resume download (try %d/3)", retry + 1);
                
                // 关闭并重新打开连接，尝试断点续传
                esp_http_client_close(client);
                vTaskDelay(pdMS_TO_TICKS(2000));
                
                // 重新打开连接，设置Range头
                char range_header[64];
                snprintf(range_header, sizeof(range_header), "bytes=%d-", total_bytes_received);
                esp_http_client_set_header(client, "Range", range_header);
                
                err = esp_http_client_open(client, 0);
                if (err == ESP_OK) {
                    esp_http_client_fetch_headers(client);
                    ESP_LOGI(TAG, "Resumed successfully, continuing from byte %d", total_bytes_received);
                    break;
                }
                
                retry++;
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            
            if (retry >= 3) {
                ESP_LOGE(TAG, "Failed to resume download after %d attempts", retry);
                break;
            }
        }
    }
    
    // 检查下载是否成功
    if (total_bytes_received <= 0) {
        ESP_LOGE(TAG, "No data received, download failed");
        esp_ota_abort(update_handle);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Download completed, received %d/%d bytes", total_bytes_received, content_length);
    
    if (total_bytes_received != content_length) {
        ESP_LOGW(TAG, "Warning: Download incomplete, but trying to continue...");
    }
    
    // 完成OTA更新
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Firmware validation failed");
        } else {
            ESP_LOGE(TAG, "OTA end failed: %s", esp_err_to_name(err));
        }
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }
    
    // 设置启动分区
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(err));
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "OTA upgrade completed successfully! Restarting...");
    
    // 关闭HTTP连接
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    // 延迟3秒，确保日志输出完成
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // 重启设备
    esp_restart();
    
    vTaskDelete(NULL);
}

static void perform_ota(const char *url)
{
    ESP_LOGI(TAG, "Starting OTA from %s", url);
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 20000,
        .keep_alive_enable = false,
        .buffer_size = OTA_BUF_SIZE,
        .buffer_size_tx = 512,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return;
    }

    // 设置读取超时（必须！）
    esp_http_client_set_timeout_ms(client, 20000);

    ESP_LOGI(TAG, "Connecting to %s ...", url);
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) {
        ESP_LOGE(TAG, "Invalid content length %d", content_length);
        esp_http_client_cleanup(client);
        return;
    }
    ESP_LOGI(TAG, "OTA file size: %d bytes", content_length);

    esp_ota_handle_t ota_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition found");
        esp_http_client_cleanup(client);
        return;
    }

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }

    char *ota_buf = malloc(OTA_BUF_SIZE);
    if (ota_buf == NULL) {
        ESP_LOGE(TAG, "No memory for OTA buffer");
        esp_ota_abort(ota_handle);
        esp_http_client_cleanup(client);
        return;
    }

    int total_read = 0;
    int last_percent = -1;
    int retry_count = 0;
    const int max_retries = 5;

    while (1) {
        int data_read = esp_http_client_read(client, ota_buf, OTA_BUF_SIZE);
        if (data_read > 0) {
            retry_count = 0;
            err = esp_ota_write(ota_handle, ota_buf, data_read);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(err));
                break;
            }
            total_read += data_read;
            int percent = (total_read * 100) / content_length;
            if (percent - last_percent >= 10 || total_read - last_percent * content_length / 100 > 100 * 1024) {
                ESP_LOGI(TAG, "OTA progress: %d%% (%d/%d)", percent, total_read, content_length);
                last_percent = percent;
            }
        } else if (data_read == 0) {
            if (total_read == content_length) {
                ESP_LOGI(TAG, "OTA download complete, total %d bytes", total_read);
                break;
            } else {
                ESP_LOGE(TAG, "Connection closed prematurely: expected %d, got %d",
                         content_length, total_read);
                break;
            }
        } else {
            esp_err_t err_code = data_read;
            if (err_code == ESP_ERR_HTTP_EAGAIN) {
                retry_count++;
                if (retry_count <= max_retries) {
                    ESP_LOGW(TAG, "OTA read EAGAIN, retry %d/%d after 1s", retry_count, max_retries);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                } else {
                    ESP_LOGE(TAG, "OTA read EAGAIN, max retries exceeded");
                    break;
                }
            } else {
                ESP_LOGE(TAG, "OTA read error: %s (%d)", esp_err_to_name(err_code), err_code);
                break;
            }
        }
    }

    free(ota_buf);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (err == ESP_OK && total_read == content_length) {
        err = esp_ota_end(ota_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
            return;
        }
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
            return;
        }
        ESP_LOGI(TAG, "OTA success, restarting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA failed, abort");
        esp_ota_abort(ota_handle);
    }
}

/* ------------------------- 解析服务器响应 ------------------------- */
static int parse_upgserver_info(const char *rsp_json)
{
    int upg_mode = UPG_IDLE;
    cJSON *root = cJSON_Parse(rsp_json);
    if (root == NULL) {
        ESP_LOGE(TAG, "JSON parse failed");
        return upg_mode;
    }
    // ESP_LOGI(TAG, "JSON parse success, root: %s", cJSON_Print(root));
    cJSON *protocol = cJSON_GetObjectItem(root, "protocol");
    if (!protocol || !cJSON_IsString(protocol) || 
        strncmp(protocol->valuestring, CLOUD_UPG_VERSION, strlen(CLOUD_UPG_VERSION)) != 0) {
        cJSON_Delete(root);
        return UPG_IDLE;
    }

    cJSON *mode = cJSON_GetObjectItem(root, "mode");
    if (mode) {
        if (cJSON_IsNumber(mode)) {
            upg_mode = mode->valueint;
        } else if (cJSON_IsString(mode)) {
            upg_mode = atoi(mode->valuestring);
        }
    }

    ESP_LOGI(TAG, "mode: %d", upg_mode);
    if (upg_mode == UPG_IDLE) {
        nvs_set_int_val(NVS_KEY_STATUS, 2);
        cJSON_Delete(root);
        return upg_mode;
    }

    char dl_url[256] = {0};
    char magicid[64] = {0};
    char version[32] = {0};
    char svn[32] = {0};
    char type[32] = {0};
    char vendorcode[32] = {0};

    cJSON *url = cJSON_GetObjectItem(root, "url");
    if (url && cJSON_IsString(url)) strlcpy(dl_url, url->valuestring, sizeof(dl_url));

    cJSON *magic = cJSON_GetObjectItem(root, "magicid");
    if (magic && cJSON_IsString(magic)) strlcpy(magicid, magic->valuestring, sizeof(magicid));

    cJSON *ver = cJSON_GetObjectItem(root, "version");
    if (ver && cJSON_IsString(ver)) strlcpy(version, ver->valuestring, sizeof(version));

    cJSON *svn_obj = cJSON_GetObjectItem(root, "svn");
    if (svn_obj && cJSON_IsString(svn_obj)) strlcpy(svn, svn_obj->valuestring, sizeof(svn));

    cJSON *type_obj = cJSON_GetObjectItem(root, "type");
    if (type_obj && cJSON_IsString(type_obj)) strlcpy(type, type_obj->valuestring, sizeof(type));

    cJSON *vc_obj = cJSON_GetObjectItem(root, "vendorcode");
    if (vc_obj && cJSON_IsString(vc_obj)) strlcpy(vendorcode, vc_obj->valuestring, sizeof(vendorcode));

    char ver_full[64];
    snprintf(ver_full, sizeof(ver_full), "%s.%s", version, svn);
    nvs_set_str_val(NVS_KEY_VERSION, ver_full);
    nvs_set_str_val(NVS_KEY_MAGICID, magicid);
    nvs_set_str_val(NVS_KEY_URL, dl_url);
    nvs_set_str_val(NVS_KEY_VENDORCODE, vendorcode);

    if (upg_mode == UPG_AUTO) {
        ESP_LOGI(TAG, "-------------START-----------------------\r\n");
        char aprule[16] = {0}, updatetime[16] = {0};
        cJSON *rule = cJSON_GetObjectItem(root, "aprule");
        if (rule && cJSON_IsString(rule)) strlcpy(aprule, rule->valuestring, sizeof(aprule));
        cJSON *time_item = cJSON_GetObjectItem(root, "time");
        if (time_item) {
            if (cJSON_IsNumber(time_item)) {
                snprintf(updatetime, sizeof(updatetime), "%d", time_item->valueint);
            } else if (cJSON_IsString(time_item)) {
                strlcpy(updatetime, time_item->valuestring, sizeof(updatetime));
            }
        }

        nvs_set_str_val(NVS_KEY_APRULE, aprule);
        nvs_set_str_val(NVS_KEY_UPTIME, updatetime);
        nvs_set_int_val(NVS_KEY_STATUS, 5);

        ESP_LOGI(TAG, "Auto upgrade triggered, rule=%s, time=%s", aprule, updatetime);
        http_ota_task(dl_url);
    } else {
        nvs_set_int_val(NVS_KEY_STATUS, 4);
    }

    cJSON_Delete(root);
    return upg_mode;
}

/* ------------------------- 云端检测主逻辑 ------------------------- */
static void cloud_update_task(void *pvParameters)
{
    // ---------- 1. 阻塞等待网络连接 ----------
    ESP_LOGI(TAG, "Waiting for network...");
    while (!ec200x_is_network_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "Cloud update task started");

    // ---------- 2. 从 NVS 读取服务器配置（失败则使用默认值）----------
    nvs_get_str_def(NVS_KEY_HOST, g_cloud_host, sizeof(g_cloud_host), DEFAULT_HOST);
    nvs_get_str_def(NVS_KEY_PATH, g_cloud_path, sizeof(g_cloud_path), DEFAULT_PATH);
    nvs_get_int_val(NVS_KEY_PORT, &g_cloud_port, DEFAULT_PORT);

    // ---------- 3. 时间戳检查（防频繁请求，此处仅打印，不跳过）----------
    uint32_t now_sec;
    if (need_update_stamp(&now_sec) == 0) {
        ESP_LOGI(TAG, "Update locked, skip this time");
        // 如需强制跳过，取消下面注释
        // vTaskDelete(NULL);
        // return;
    }

    // ---------- 4. 构建 HTTP 请求头与 Body ----------
    char http_h[512] = {0};
    char http_b[1536] = {0};
    if (http_post_ready(http_h, http_b, sizeof(http_b)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to build HTTP request");
        vTaskDelete(NULL);
        return;
    }

    // ---------- 5. 手动 HTTP POST 事务（不依赖 esp_http_client_perform）----------
    char full_url[256];
    snprintf(full_url, sizeof(full_url), "http://%s:%d%s",
             g_cloud_host, g_cloud_port, g_cloud_path);

    esp_http_client_config_t cfg = {
        .url = full_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = SOCK_RW_TIMEOUT * 1000,
        .buffer_size = 1024,           // 接收缓冲区大小
        .keep_alive_enable = false,    // 单次事务，不保持连接
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        vTaskDelete(NULL);
        return;
    }

    // 设置 Content-Type 头
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // 打开连接，指定待发送的 body 长度
    esp_err_t err = esp_http_client_open(client, strlen(http_b));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }

    // 写入 POST body
    int wlen = esp_http_client_write(client, http_b, strlen(http_b));
    if (wlen < 0) {
        ESP_LOGE(TAG, "Write failed: %d", wlen);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
        return;
    }

    // 获取响应头（含状态码和 Content-Length）
    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP status = %d, content-length = %d", status_code, content_length);

    if (status_code == 200) {
        // 分配响应体缓冲区（优先使用 Content-Length，否则用默认大小）
        int buf_size = MAX_HTTP_OUTPUT_BUFFER;
        if (content_length > 0 && content_length < MAX_HTTP_OUTPUT_BUFFER) {
            buf_size = content_length + 1;
        }
        char *rsp_buf = malloc(buf_size);
        if (rsp_buf == NULL) {
            ESP_LOGE(TAG, "Failed to allocate response buffer");
        } else {
            int total_read = 0;
            int read_len;
            // 循环读取响应体，直到读完或缓冲区满
            do {
                read_len = esp_http_client_read(client,
                                                rsp_buf + total_read,
                                                buf_size - total_read - 1);
                if (read_len > 0) {
                    total_read += read_len;
                }
            } while (read_len > 0 && total_read < buf_size - 1);

            if (total_read > 0) {
                rsp_buf[total_read] = '\0';
                ESP_LOGI(TAG, "Response: %s", rsp_buf);
                parse_upgserver_info(rsp_buf);
            } else {
                ESP_LOGW(TAG, "No response body received");
            }
            free(rsp_buf);
        }
    } else {
        ESP_LOGW(TAG, "Unexpected HTTP status: %d", status_code);
    }

    // 关闭并清理 HTTP 客户端
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    // ---------- 6. 更新成功时间戳 ----------
    exe_update_stamp(now_sec);

    ESP_LOGI(TAG, "Cloud update task finished, deleting itself");
    vTaskDelete(NULL);
}

/* ------------------------- 公共启动函数 ------------------------- */
void start_cloud_update(void)
{
    meminfo_t mt;
    check_machine_memory(&mt);
    uint32_t flash_size = get_flash_total_size();
    if (mt.free < flash_size) {
        ESP_LOGW(TAG, "Free memory %lu < flash size %lu, skip OTA", mt.free, flash_size);
    }

    xTaskCreate(cloud_update_task, "cloud_upd", 8192, NULL, 5, NULL);
}












// #include "esp_http_client.h"
// #include "esp_log.h"
// #include "esp_ota_ops.h"
// #include "esp_flash_partitions.h"
// #include "nvs_flash.h"
// #include "cloudupdate.h"
// #include <string.h>
// #include <sys/time.h>

// static const char *TAG = "OTA";

// // OTA状态结构体
// typedef struct {
//     int total_size;
//     int downloaded;
//     int retry_count;
//     bool download_complete;
//     bool write_complete;
//     bool validated;
// } ota_status_t;

// // 全局OTA状态
// static ota_status_t g_ota_status = {0};

// // 获取当前时间（毫秒）
// static int64_t get_time_ms(void)
// {
//     struct timeval tv;
//     gettimeofday(&tv, NULL);
//     return (tv.tv_sec * 1000LL + tv.tv_usec / 1000);
// }

// // 保存OTA进度到NVS
// static esp_err_t save_ota_progress(int progress, int total_size)
// {
//     nvs_handle_t nvs_handle;
//     esp_err_t err = nvs_open("ota_progress", NVS_READWRITE, &nvs_handle);
//     if (err != ESP_OK) {
//         return err;
//     }
    
//     err = nvs_set_i32(nvs_handle, "progress", progress);
//     if (err == ESP_OK) {
//         err = nvs_set_i32(nvs_handle, "total_size", total_size);
//     }
    
//     nvs_commit(nvs_handle);
//     nvs_close(nvs_handle);
//     return err;
// }

// // 读取OTA进度
// static esp_err_t read_ota_progress(int *progress, int *total_size)
// {
//     nvs_handle_t nvs_handle;
//     esp_err_t err = nvs_open("ota_progress", NVS_READONLY, &nvs_handle);
//     if (err != ESP_OK) {
//         return err;
//     }
    
//     err = nvs_get_i32(nvs_handle, "progress", progress);
//     if (err == ESP_OK) {
//         err = nvs_get_i32(nvs_handle, "total_size", total_size);
//     }
    
//     nvs_close(nvs_handle);
//     return err;
// }

// // 清除OTA进度
// static void clear_ota_progress(void)
// {
//     nvs_handle_t nvs_handle;
//     if (nvs_open("ota_progress", NVS_READWRITE, &nvs_handle) == ESP_OK) {
//         nvs_erase_all(nvs_handle);
//         nvs_commit(nvs_handle);
//         nvs_close(nvs_handle);
//     }
// }

// // 检查网络连接状态，如果有问题尝试重新连接
// static bool ensure_network_connected(void)
// {
//     static int network_check_count = 0;
    
//     if (ec200x_is_network_connected()) {
//         network_check_count = 0;
//         return true;
//     }
    
//     network_check_count++;
//     ESP_LOGW(TAG, "网络断开，尝试重新连接 (%d)", network_check_count);
    
//     // 等待网络恢复
//     int wait_time = 0;
//     while (wait_time < 30) { // 最多等待30秒
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         wait_time++;
        
//         if (ec200x_is_network_connected()) {
//             ESP_LOGI(TAG, "网络重新连接成功");
//             network_check_count = 0;
//             return true;
//         }
//     }
    
//     ESP_LOGE(TAG, "网络连接失败");
//     return false;
// }

// // 下载固件片段（支持断点续传）
// static esp_err_t download_firmware_segment(esp_http_client_handle_t client, 
//                                           esp_ota_handle_t update_handle,
//                                           int start_byte, 
//                                           int segment_size)
// {
//     char buffer[4096];  // 4KB缓冲区
//     int bytes_received = 0;
//     int64_t segment_start_time = get_time_ms();
//     int last_report_time = 0;
    
//     // 设置Range头
//     char range_header[64];
//     snprintf(range_header, sizeof(range_header), "bytes=%d-%d", 
//              start_byte, start_byte + segment_size - 1);
//     esp_http_client_set_header(client, "Range", range_header);
    
//     // 重新打开连接
//     esp_http_client_close(client);
//     esp_err_t err = esp_http_client_open(client, 0);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "重新打开连接失败: %s", esp_err_to_name(err));
//         return err;
//     }
    
//     esp_http_client_fetch_headers(client);
    
//     // 下载片段
//     while (bytes_received < segment_size) {
//         if (!ensure_network_connected()) {
//             return ESP_FAIL;
//         }
        
//         int to_read = segment_size - bytes_received;
//         if (to_read > sizeof(buffer)) {
//             to_read = sizeof(buffer);
//         }
        
//         int read_len = esp_http_client_read(client, buffer, to_read);
//         if (read_len > 0) {
//             err = esp_ota_write(update_handle, buffer, read_len);
//             if (err != ESP_OK) {
//                 ESP_LOGE(TAG, "写入固件数据失败: %s", esp_err_to_name(err));
//                 return err;
//             }
            
//             bytes_received += read_len;
//             g_ota_status.downloaded += read_len;
            
//             // 每5秒报告一次进度
//             int64_t current_time = get_time_ms();
//             if (current_time - segment_start_time > 5000) {
//                 int percent = (g_ota_status.downloaded * 100) / g_ota_status.total_size;
//                 int speed = (bytes_received * 1000) / (current_time - segment_start_time);
                
//                 ESP_LOGI(TAG, "进度: %d/%d (%d%%) 速度: %d B/s", 
//                         g_ota_status.downloaded, g_ota_status.total_size, 
//                         percent, speed);
                
//                 // 保存进度
//                 save_ota_progress(g_ota_status.downloaded, g_ota_status.total_size);
                
//                 segment_start_time = current_time;
//                 bytes_received = 0; // 重置统计
//             }
//         } 
//         else if (read_len == 0) {
//             // 短暂等待
//             vTaskDelay(pdMS_TO_TICKS(100));
//         } 
//         else {
//             ESP_LOGE(TAG, "读取失败: %d", read_len);
//             return ESP_FAIL;
//         }
//     }
    
//     return ESP_OK;
// }

// // HTTP OTA任务
// static void http_ota_task(void *pvParameter)
// {
//     esp_err_t err;
//     const esp_partition_t *update_partition = NULL;
//     esp_ota_handle_t update_handle = 0;
    
//     // 初始化OTA状态
//     memset(&g_ota_status, 0, sizeof(g_ota_status));
    
//     // 检查是否有未完成的OTA
//     int saved_progress = 0;
//     int saved_total_size = 0;
//     if (read_ota_progress(&saved_progress, &saved_total_size) == ESP_OK && saved_progress > 0) {
//         ESP_LOGI(TAG, "发现未完成的OTA: %d/%d bytes", saved_progress, saved_total_size);
//         ESP_LOGI(TAG, "是否继续? (继续下载需要网络连接稳定)");
//     }
    
//     ESP_LOGI(TAG, "等待网络连接...");
    
//     // 等待网络连接，超时5分钟
//     int64_t network_wait_start = get_time_ms();
//     while (!ec200x_is_network_connected()) {
//         vTaskDelay(pdMS_TO_TICKS(1000));
        
//         if (get_time_ms() - network_wait_start > 300000) { // 5分钟超时
//             ESP_LOGE(TAG, "等待网络连接超时");
//             vTaskDelete(NULL);
//             return;
//         }
//     }
    
//     ESP_LOGI(TAG, "网络已连接");
    
//     const char *url = "http://120.55.37.245:9000/otatest/temp/esp32-s3-remoteid_1770954045047.bin";
//     ESP_LOGI(TAG, "开始HTTP OTA升级，URL: %s", url);
    
//     // 配置HTTP客户端
//     esp_http_client_config_t config = {
//         .url = url,
//         .method = HTTP_METHOD_GET,
//         .timeout_ms = 30000,           // 30秒超时
//         .buffer_size = 4096,
//         .keep_alive_enable = true,
//         .max_redirection_count = 3,
//         .user_agent = "ESP32-OTA/1.0",
//     };
    
//     esp_http_client_handle_t client = esp_http_client_init(&config);
//     if (client == NULL) {
//         ESP_LOGE(TAG, "初始化HTTP客户端失败");
//         vTaskDelete(NULL);
//         return;
//     }
    
//     // 获取文件大小
//     esp_http_client_set_method(client, HTTP_METHOD_HEAD);
//     err = esp_http_client_open(client, 0);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "HEAD请求失败: %s", esp_err_to_name(err));
//         esp_http_client_cleanup(client);
//         vTaskDelete(NULL);
//         return;
//     }
    
//     int status_code = esp_http_client_get_status_code(client);
//     if (status_code != 200) {
//         ESP_LOGE(TAG, "服务器响应错误: %d", status_code);
//         esp_http_client_cleanup(client);
//         vTaskDelete(NULL);
//         return;
//     }
    
//     int content_length = esp_http_client_get_content_length(client);
//     if (content_length <= 0) {
//         ESP_LOGE(TAG, "无法获取文件大小");
//         esp_http_client_cleanup(client);
//         vTaskDelete(NULL);
//         return;
//     }
    
//     ESP_LOGI(TAG, "固件大小: %d bytes", content_length);
//     g_ota_status.total_size = content_length;
    
//     // 获取OTA分区
//     update_partition = esp_ota_get_next_update_partition(NULL);
//     if (update_partition == NULL) {
//         ESP_LOGE(TAG, "找不到OTA分区");
//         esp_http_client_cleanup(client);
//         vTaskDelete(NULL);
//         return;
//     }
    
//     ESP_LOGI(TAG, "写入分区: %s (大小: %d)", 
//              update_partition->label, update_partition->size);
    
//     // 验证分区大小
//     if (content_length > update_partition->size) {
//         ESP_LOGE(TAG, "固件太大！");
//         esp_http_client_cleanup(client);
//         vTaskDelete(NULL);
//         return;
//     }
    
//     // 开始OTA
//     err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "OTA开始失败: %s", esp_err_to_name(err));
//         esp_http_client_cleanup(client);
//         vTaskDelete(NULL);
//         return;
//     }
    
//     // 分块下载（每块256KB，减少单次连接时间）
//     const int CHUNK_SIZE = 256 * 1024;
//     int remaining = content_length;
//     int start_pos = 0;
//     int chunk_count = 0;
    
//     ESP_LOGI(TAG, "开始分块下载...");
    
//     while (remaining > 0) {
//         int chunk_size = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
//         chunk_count++;
        
//         ESP_LOGI(TAG, "下载块 %d: 字节 %d-%d (%d bytes)", 
//                 chunk_count, start_pos, start_pos + chunk_size - 1, chunk_size);
        
//         err = download_firmware_segment(client, update_handle, start_pos, chunk_size);
//         if (err != ESP_OK) {
//             ESP_LOGE(TAG, "块 %d 下载失败", chunk_count);
//             esp_ota_abort(update_handle);
//             break;
//         }
        
//         start_pos += chunk_size;
//         remaining -= chunk_size;
        
//         ESP_LOGI(TAG, "块 %d 下载完成，剩余 %d bytes", chunk_count, remaining);
//     }
    
//     esp_http_client_close(client);
//     esp_http_client_cleanup(client);
    
//     if (remaining > 0) {
//         ESP_LOGE(TAG, "下载不完整");
//         esp_ota_abort(update_handle);
//         vTaskDelete(NULL);
//         return;
//     }
    
//     ESP_LOGI(TAG, "下载完成，验证固件...");
    
//     // 完成OTA
//     err = esp_ota_end(update_handle);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "OTA结束失败: %s", esp_err_to_name(err));
//         vTaskDelete(NULL);
//         return;
//     }
    
//     // 验证固件
//     const esp_partition_t *running = esp_ota_get_running_partition();
//     const esp_partition_t *new_firmware = esp_ota_get_next_update_partition(NULL);
    
//     ESP_LOGI(TAG, "当前运行分区: %s", running->label);
//     ESP_LOGI(TAG, "新固件分区: %s", new_firmware->label);
    
//     // 设置启动分区
//     err = esp_ota_set_boot_partition(new_firmware);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "设置启动分区失败: %s", esp_err_to_name(err));
//         vTaskDelete(NULL);
//         return;
//     }
    
//     // 清除进度记录
//     clear_ota_progress();
    
//     ESP_LOGI(TAG, "OTA升级成功！准备重启...");
    
//     // 保存关键日志
//     esp_log_level_set("*", ESP_LOG_INFO);
    
//     // 延迟重启，确保日志输出完成
//     for (int i = 3; i > 0; i--) {
//         ESP_LOGI(TAG, "将在 %d 秒后重启...", i);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
    
//     // 重启设备
//     esp_restart();
    
//     vTaskDelete(NULL);
// }

// // 启动OTA升级
// void start_cloud_update(void)
// {
//     // 检查是否有足够的堆内存
//     size_t free_heap = esp_get_free_heap_size();
//     ESP_LOGI(TAG, "可用堆内存: %d bytes", free_heap);
    
//     if (free_heap < 50000) {
//         ESP_LOGW(TAG, "堆内存不足，可能无法进行OTA");
//     }
    
//     // 创建OTA任务
//     xTaskCreate(http_ota_task, "ota_task", 24576, NULL, 5, NULL);
// }

// // 检查并初始化NVS
// void ota_init(void)
// {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);
    
//     ESP_LOGI(TAG, "OTA模块初始化完成");
// }