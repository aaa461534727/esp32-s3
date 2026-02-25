#include "web_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "global.h"
#include <esp_http_server.h>
#include <esp_ota_ops.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <string.h>
#include <sys/param.h>
#include "mbedtls/sha256.h"

static const char *TAG = "web_server";
static esp_ota_handle_t ota_handle = 0;

static bool s_ota_in_progress = false;   // OTA正在进行则为true
// 查询当前OTA是否正在进行（非阻塞）
bool is_ota_in_progress(void)
{
    return s_ota_in_progress;
}

/* 单个文件的最大大小*/
#define MAX_FILE_SIZE   (1000*1024) // 1000 KB
#define MAX_FILE_SIZE_STR "1000KB"
/* 暂存缓冲区大小*/
#define SCRATCH_BUFSIZE  1024
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BUFFER_LEN  1024  
//定义接收发送的数据结构体
typedef struct 
{
    char data[BUFFER_LEN];
    int len;
    int client_socket;
}DATA_PARCEL;
//解析数据存放
//初始化wbe全局变量，确保内存分配
WebCommand_t webdata = {
    .basic_id = "",
    .custom_desc = "",
    .run_desc = "",
    .operator_info = "",
    .client_socket = 0,
};
static httpd_handle_t web_server_handle = NULL;//ws服务器唯一句柄
static QueueHandle_t  ws_server_rece_queue = NULL;//收到的消息传给任务处理
static QueueHandle_t  ws_server_send_queue = NULL;//异步发送队列

/*此处只是管理ws socket server发送时的对象，以确保多客户端连接的时候都能收到数据，并不能限制HTTP请求*/
#define WS_CLIENT_QUANTITY_ASTRICT 5    //客户端数量
static int WS_CLIENT_LIST[WS_CLIENT_QUANTITY_ASTRICT];//客户端套接字列表
static int WS_CLIENT_NUM = 0;   //实际连接数量

/*客户端列表 记录客户端套接字*/
static void ws_client_list_add(int socket)
{
    /*检查是否超出限制*/
    if (WS_CLIENT_NUM>=WS_CLIENT_QUANTITY_ASTRICT)
    {
        return;
    }
    
    /*检查是否重复*/
    for (size_t i = 0; i < WS_CLIENT_QUANTITY_ASTRICT; i++) 
    {
        if (WS_CLIENT_LIST[i] == socket) {
            return;
        } 
    }

    /*添加套接字至列表中*/
    for (size_t i = 0; i < WS_CLIENT_QUANTITY_ASTRICT; i++) 
    {
        if (WS_CLIENT_LIST[i] <= 0){
            WS_CLIENT_LIST[i] = socket; //获取返回信息的客户端套接字
            printf("ws_client_list_add:%d\r\n",socket);
            WS_CLIENT_NUM++;
            return;
        }
    }
}

/*客户端列表 删除客户端套接字*/
static void ws_client_list_delete(int socket)
{
    for (size_t i = 0; i < WS_CLIENT_QUANTITY_ASTRICT; i++)
    {
        if (WS_CLIENT_LIST[i] == socket)
        {
            WS_CLIENT_LIST[i] = 0;
            printf("ws_client_list_delete:%d\r\n",socket);
            WS_CLIENT_NUM--;
            if (WS_CLIENT_NUM<0)
            {
                WS_CLIENT_NUM = 0;
            }
            break;
        }
    }
}

/*ws服务器接收原始数据数据*/
static DATA_PARCEL ws_rece_parcel;  
static esp_err_t ws_server_rece_data(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ws_client_list_add(httpd_req_to_sockfd(req));
        return ESP_OK;
    }
    esp_err_t ret = ESP_FAIL;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    memset(&ws_rece_parcel, 0, sizeof(DATA_PARCEL));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = (uint8_t*)ws_rece_parcel.data;   //指向缓存区
    ret = httpd_ws_recv_frame(req, &ws_pkt, 0);//设置参数max_len = 0来获取帧长度
    if (ret != ESP_OK) {
        printf("ws_server_rece_data data receiving failure!");
        return ret;
    }
    if (ws_pkt.len>0) 
    {
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);/*设置参数max_len 为 ws_pkt.len以获取帧有效负载 */
        if (ret != ESP_OK) {
            printf("ws_server_rece_data data receiving failure!");
            return ret;
        }
        ws_rece_parcel.len = ws_pkt.len;
        ws_rece_parcel.client_socket = httpd_req_to_sockfd(req);
        if (xQueueSend(ws_server_rece_queue ,&ws_rece_parcel,pdMS_TO_TICKS(1))){
            ret = ESP_OK;
        }
    }
    else 
    {
        printf("ws_pkt.len<=0");
    }
    return ret;
}

uint8_t Upload_Timeout_num;
/* OTA文件上传处理 */
static esp_err_t upload_post_handler(httpd_req_t *req) 
{
    free_iot_init();
    udp_client_free();
    s_ota_in_progress = true;
    esp_ota_handle_t ota_handle = 0;
    char *buffer = malloc(2048); // 增大缓冲区至2KB
    int received, content_length = req->content_len;
    bool is_header_parsed = false;
    char *boundary = NULL;

    // 步骤1：检测Content-Type并提取boundary
    char content_type[128] = {0};
    if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type)) == ESP_OK) {
        boundary = strstr(content_type, "boundary=");
        if (boundary) {
            boundary += strlen("boundary=");
            ESP_LOGI(TAG, "Boundary detected: %s", boundary);
        }
    }

    // 步骤2：获取OTA分区并初始化
    const esp_partition_t* update_part = esp_ota_get_next_update_partition(NULL);
    if (!update_part) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA Partition");
        free(buffer);
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(esp_ota_begin(update_part, OTA_SIZE_UNKNOWN, &ota_handle));

    // 步骤3：循环接收并处理数据
    while (content_length > 0) {
        received = httpd_req_recv(req, buffer, MIN(content_length, 2048));
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) continue;
            ESP_LOGE(TAG, "接收失败: %d", received);
            esp_ota_abort(ota_handle);
            free(buffer);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "接收中断");
            return ESP_FAIL;
        }

        // 关键修改：跳过multipart头部
        if (!is_header_parsed) {
            char *data_start = strstr(buffer, "\r\n\r\n");
            if (data_start) {
                data_start += 4; // 跳过头部结束符
                int payload_len = received - (data_start - buffer);
                // 强制校验Magic Byte
                if (payload_len >=4 && data_start[0] != 0xE9) {
                    ESP_LOGE(TAG, "固件头校验失败: 0x%02X", data_start[0]);
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid firmware");
                    return ESP_FAIL;
                }
                esp_ota_write(ota_handle, data_start, payload_len);
                content_length -= (data_start - buffer) + payload_len;
                is_header_parsed = true;
            } else {
                content_length -= received;
            }
        } else {
            esp_ota_write(ota_handle, buffer, received);
            content_length -= received;
        }
    }

    // 步骤4：完成OTA并重启
    if (esp_ota_end(ota_handle) == ESP_OK) {
        esp_ota_set_boot_partition(update_part);
        httpd_resp_sendstr(req, "升级成功，即将重启...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        esp_restart();
    }
    free(buffer);
    return ESP_OK;
}

/*首页HTML GET处理程序（对应新的 home.html，需嵌入为 index.html） */
static esp_err_t home_get_handler(httpd_req_t *req)
{
    /*获取脚本index.html的存放地址和大小，接受http请求时将脚本发出去*/
    extern const unsigned char home_html_start[] asm("_binary_home_html_start");
    extern const unsigned char home_html_end[]   asm("_binary_home_html_end");
    const size_t size = (home_html_end - home_html_start);
    httpd_resp_send(req, (const char *)home_html_start, size);
    return ESP_OK;
}

/* 新增：manage.html GET处理程序 */
static esp_err_t manage_get_handler(httpd_req_t *req)
{
    extern const unsigned char manage_html_start[] asm("_binary_manage_html_start");
    extern const unsigned char manage_html_end[]   asm("_binary_manage_html_end");
    const size_t size = (manage_html_end - manage_html_start);
    httpd_resp_send(req, (const char *)manage_html_start, size);
    return ESP_OK;
}

/* 新增：display.html GET处理程序 */
static esp_err_t display_get_handler(httpd_req_t *req)
{
    extern const unsigned char display_html_start[] asm("_binary_display_html_start");
    extern const unsigned char display_html_end[]   asm("_binary_display_html_end");
    const size_t size = (display_html_end - display_html_start);
    httpd_resp_send(req, (const char *)display_html_start, size);
    return ESP_OK;
}
/* 处理 /data 的 GET 请求，返回最新接收的数据 */
static esp_err_t data_get_handler(httpd_req_t *req)
{
    char buffer[2048] = {0};
    int len = rid_wifi_sniffer_get_last_data(buffer, sizeof(buffer));
    if (len > 0) {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr(req, buffer);
    }
    return ESP_OK;
}

/* 新增：ota.html GET处理程序 */
static esp_err_t ota_get_handler(httpd_req_t *req)
{
    extern const unsigned char ota_html_start[] asm("_binary_ota_html_start");
    extern const unsigned char ota_html_end[]   asm("_binary_ota_html_end");
    const size_t size = (ota_html_end - ota_html_start);
    httpd_resp_send(req, (const char *)ota_html_start, size);
    return ESP_OK;
}

/*图标回调*/
static esp_err_t favicon_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, NULL, 0); // 返回空图标
    return ESP_OK;
}

/*WEB SOCKET*/
static const httpd_uri_t ws = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = ws_server_rece_data,
    .user_ctx   = NULL,
    .is_websocket = true
};

/*http post请求*/
static const httpd_uri_t upload = {
    .uri       = "/update",
    .method    = HTTP_POST,
    .handler   = upload_post_handler
};
/*首页HTML*/
static const httpd_uri_t home = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = home_get_handler,
    .user_ctx  = NULL
};

/* 新增：manage.html 路由 */
static const httpd_uri_t manage = {
    .uri       = "/manage.html",
    .method    = HTTP_GET,
    .handler   = manage_get_handler,
    .user_ctx  = NULL
};

/* 新增：display.html 路由 */
static const httpd_uri_t display = {
    .uri       = "/display.html",
    .method    = HTTP_GET,
    .handler   = display_get_handler,
    .user_ctx  = NULL
};

/* 新增：ota.html 路由 */
static const httpd_uri_t ota = {
    .uri       = "/ota.html",
    .method    = HTTP_GET,
    .handler   = ota_get_handler,
    .user_ctx  = NULL
};

/* 新增：数据获取接口 */
static const httpd_uri_t data_get = {
    .uri       = "/data",
    .method    = HTTP_GET,
    .handler   = data_get_handler,
    .user_ctx  = NULL
};

/*http post请求*/
static const httpd_uri_t icon = {
    .uri       = "/favicon.ico",
    .method    = HTTP_GET,
    .handler   = favicon_handler,
};

/*http事件处理*/
static void ws_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if (event_base == ESP_HTTP_SERVER_EVENT )
    {
        switch (event_id)
        {
            case HTTP_SERVER_EVENT_ERROR ://当执行过程中出现错误时，将发生此事件
                break;
            case HTTP_SERVER_EVENT_START  ://此事件在HTTP服务器启动时发生
                break;
            case HTTP_SERVER_EVENT_ON_CONNECTED  ://一旦HTTP服务器连接到客户端，就不会执行任何数据交换
                break;
            case HTTP_SERVER_EVENT_ON_HEADER  ://在接收从客户端发送的每个报头时发生
                break;
            case HTTP_SERVER_EVENT_HEADERS_SENT  ://在将所有标头发送到客户端之后
                break;
            case HTTP_SERVER_EVENT_ON_DATA  ://从客户端接收数据时发生
                break;
            case HTTP_SERVER_EVENT_SENT_DATA ://当ESP HTTP服务器会话结束时发生
                break;
            case HTTP_SERVER_EVENT_DISCONNECTED  ://连接已断开
                esp_http_server_event_data* event = (esp_http_server_event_data*)event_data;
                ws_client_list_delete(event->fd);
                break;
            case HTTP_SERVER_EVENT_STOP   ://当HTTP服务器停止时发生此事件
                break;
        }
    }
}

//发送函数主要是给连接esp32的客户端发消息
/*异步发送函数，将其放入HTTPD工作队列*/
static DATA_PARCEL async_buffer;
static void ws_async_send(void *arg)
{
    if (xQueueReceive(ws_server_send_queue,&async_buffer,0))
    {
        httpd_ws_frame_t ws_pkt ={0};
        ws_pkt.payload = (uint8_t*)async_buffer.data;
        ws_pkt.len = async_buffer.len;
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        httpd_ws_send_frame_async(web_server_handle, async_buffer.client_socket, &ws_pkt) ;
    }
}

/*ws 发送函数*/
static DATA_PARCEL send_buffer;
static void ws_server_send(const char * data ,uint32_t len , int client_socket)
{
    memset(&send_buffer,0,sizeof(send_buffer));
    send_buffer.client_socket = client_socket;
    send_buffer.len = len;
    memcpy(send_buffer.data,data,len);
    xQueueSend(ws_server_send_queue ,&send_buffer,pdMS_TO_TICKS(1));
    httpd_queue_work(web_server_handle, ws_async_send, NULL);//进入排队
}

/*数据发送任务，每隔一秒发送一次*/
static void ws_server_send_task(void *p)
{
    uint32_t task_count = 0;
    char buf[50] ;
    while (1)
    {
        memset(buf,0,sizeof(buf));
        sprintf(buf,"Hello World! %ld",task_count);

        for (size_t i = 0; i < WS_CLIENT_QUANTITY_ASTRICT; i++)
        {
            if (WS_CLIENT_LIST[i]>0)
            {
                ws_server_send(buf,strlen(buf),WS_CLIENT_LIST[i]);
            } 
        }
        task_count++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* 新增数据解析函数 */
static void parse_web_command(const char* data, WebCommand_t* cmd) {
    char* colon = strchr(data, ':');
    if (!colon || (colon - data) >= 32) return;

    size_t key_len = colon - data;
    char key[32] = {0};
    strncpy(key, data, key_len);
    
    const char* value = colon + 1;
    
    // 根据前端定义的字段处理
    if (strcmp(key, "BASIC_ID") == 0) {
        strncpy(cmd->basic_id, value, sizeof(cmd->basic_id)-1);
        cmd->basic_id[sizeof(cmd->basic_id)-1] = '\0';
    } 
    else if (strcmp(key, "CUSTOM_DESC") == 0) {
        strncpy(cmd->custom_desc, value, sizeof(cmd->custom_desc)-1);
        cmd->custom_desc[sizeof(cmd->custom_desc)-1] = '\0';
    }
    else if (strcmp(key, "RUN_DESC") == 0) {
        strncpy(cmd->run_desc, value, sizeof(cmd->run_desc)-1);
        cmd->run_desc[sizeof(cmd->run_desc)-1] = '\0';
    }
    else if (strcmp(key, "OPERATOR") == 0) {
        strncpy(cmd->operator_info, value, sizeof(cmd->operator_info)-1);
        cmd->operator_info[sizeof(cmd->operator_info)-1] = '\0';
    }
}

//接收来自网页客户端的数据
/*数据接收处理任务*/
static DATA_PARCEL rece_buffer;
static void ws_server_rece_task(void *p)
{
    while (1)
    {
        if(xQueueReceive(ws_server_rece_queue,&rece_buffer,portMAX_DELAY))
        {
            //printf("socket : %d\tdata len : %d\tpayload : %s\r\n",rece_buffer.client_socket,rece_buffer.len,rece_buffer.data);
            // 数据解析处理
            // 确保字符串正确终止
            if (rece_buffer.len >= BUFFER_LEN) {
                ESP_LOGE(TAG, "Received data exceeds buffer size!");
                continue;
            }
            rece_buffer.data[rece_buffer.len] = '\0';

            // 初始化命令缓冲区
            memset(&webdata, 0, sizeof(WebCommand_t));
            webdata.client_socket = rece_buffer.client_socket;

            // 解析数据
            parse_web_command(rece_buffer.data, &webdata);

            // 打印调试信息
            ESP_LOGI(TAG, "Received from socket %d:", webdata.client_socket);
            ESP_LOGI(TAG, "  Basic ID: %s", webdata.basic_id);
            ESP_LOGI(TAG, "  Custom Desc: %s", webdata.custom_desc);
            ESP_LOGI(TAG, "  Run Desc: %s", webdata.run_desc);
            ESP_LOGI(TAG, "  Operator: %s", webdata.operator_info);
        }
    }

}

/*web服务器初始化*/
void start_web_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;  // 增加至 8KB
    // 启动httpd服务器
    if (httpd_start(&web_server_handle, &config) == ESP_OK) {
        ESP_LOGI(TAG, "web_server_start\r\n");
    }
    else {
        ESP_LOGE(TAG, "Error starting ws server!");
    }

    esp_event_handler_instance_register(ESP_HTTP_SERVER_EVENT,ESP_EVENT_ANY_ID, &ws_event_handler,NULL,NULL);//注册处理程序
    httpd_register_uri_handler(web_server_handle, &home);//注册页面处理程序
    httpd_register_uri_handler(web_server_handle, &ws);//注册接收数据处理程序
    httpd_register_uri_handler(web_server_handle, &upload);//注册ota更新处理程序
    httpd_register_uri_handler(web_server_handle, &icon);//注册图标处理程序
    /* 新增：注册模块页面处理程序 */
    httpd_register_uri_handler(web_server_handle, &manage);
    httpd_register_uri_handler(web_server_handle, &display);
    httpd_register_uri_handler(web_server_handle, &ota);
    httpd_register_uri_handler(web_server_handle, &data_get);

    /*创建接收队列*/
    ws_server_rece_queue = xQueueCreate(  3 , sizeof(DATA_PARCEL)); 
    if (ws_server_rece_queue == NULL )
    {
        ESP_LOGE(TAG, "ws_server_rece_queue ERROR\r\n");
    }

    /*创建发送队列*/
    ws_server_send_queue = xQueueCreate(  3 , sizeof(DATA_PARCEL)); 
    if (ws_server_send_queue == NULL )
    {
        ESP_LOGE(TAG, "ws_server_send_queue ERROR\r\n");
    }

    BaseType_t xReturn ;
    /*创建接收处理任务*///改创建任务的函数可以执行CPU核心0、1，当前参数不指定
    xReturn = xTaskCreatePinnedToCore(ws_server_rece_task,"ws_server_rece_task",10*1024,NULL,15,NULL, tskNO_AFFINITY);
    if(xReturn != pdPASS) 
    {
        ESP_LOGE(TAG, "xTaskCreatePinnedToCore ws_server_rece_task error!\r\n");
    }
    // /*创建发送任务，此任务不是必须的，因为发送函数可以放在任意地方*/
    // xReturn = xTaskCreatePinnedToCore(ws_server_send_task,"ws_server_send_task",4096,NULL,15,NULL, tskNO_AFFINITY);
    // if(xReturn != pdPASS) 
    // {
    //     ESP_LOGE(TAG, "xTaskCreatePinnedToCore ws_server_send_task error!\r\n");
    // }
}