#include "gps.h"

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)
static const int RX_BUF_SIZE = 1024;

//初始化gps全局变量，确保内存分配
GPS_Data_t gpsData = {
    .latitude = 0.0,
    .longitude = 0.0,
    .timestamp = "1970-01-01 00:00:00",
    .speed = 0.0,
    .satellites = 0,
    .fix_status = false
};

void uart1_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


int sendData(const char* logName, int uart_num ,const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(uart_num, data, len);
    //ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

void gps_tx1_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG,UART_NUM_1,"Hello world");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void gps_rx1_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX1_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
         //printf("\n----------RX1- %d-----\n",rxBytes);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            if (xSemaphoreTake(gps_Mutex, portMAX_DELAY) == pdTRUE) {
                // 更新 GPS 数据
                // printf("\n----------update_gps_data-----\n");
                gpsData.latitude = 31.2304;
                gpsData.longitude = 121.4737;
                strcpy(gpsData.timestamp, "2025-03-01 14:30:00");
                gpsData.speed = 5.2;
                gpsData.satellites = 8;
                gpsData.fix_status = true;
                xSemaphoreGive(gps_Mutex);  // 释放互斥信号量
            }
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    free(data);
}

void gps_Interface_init(void)
{
    uart1_init();
    BaseType_t xReturn ;
    //接收gps数据
    xReturn = xTaskCreatePinnedToCore(gps_rx1_task,"gps_rx1_task",8192,NULL,13,NULL, tskNO_AFFINITY);
    if(xReturn != pdPASS) 
    {
        printf("xTaskCreatePinnedToCore gps_rx1_task error!\r\n");
    }
    /*创建发送任务*///改创建任务的函数可以执行CPU核心0、1，当前参数不指定
    xReturn = xTaskCreatePinnedToCore(gps_tx1_task,"gps_tx1_task",4096,NULL,12,NULL, tskNO_AFFINITY);
    if(xReturn != pdPASS) 
    {
        printf("xTaskCreatePinnedToCore gps_tx1_task error!\r\n");
    }

}